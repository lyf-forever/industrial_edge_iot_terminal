# industrial_edge_iot_terminal

基于 **双 MCU** 架构的工业级边缘物联网智能终端（Industrial Edge IoT Terminal）。本仓库同时维护 **ESP32S3**、**GD32H7** 两端 MCU 固件源码与 **Android APP**，三者独立目录、可分别编译，通过统一通信契约协作。

## 目录结构

```
industrial_edge_iot_terminal/
├── README.md                                    # 本文件：仓库总览
├── LICENSE                                      # GPL-3.0 开源许可
├── Industrial_Edge_IoT_Terminal_ESP32S3/        # 【端 1】Wi-Fi/BLE 无线通信端（ESP-IDF v5.x）
├── Industrial_Edge_IoT_Terminal_GD32H7/         # 【端 2】实时采集/显示控制端（CMake + arm-none-eabi-gcc）
├── Android_APP/                                 # 【端 3】Android 原生 APP（Kotlin，MQTT/BLE/SPP/TCP）
├── .github/workflows/android.yml                # Android CI（构建+单测+lint+模拟器冒烟）
└── docs/
    ├── smart_iot_terminal_guide.docx            # 整机总体设计说明
    ├── app_firmware_contract.docx               # APP 与双端固件通信契约（主题/命令/协议草案）
    └── project_modDistribution_survey.docx      # 双端外部模块分配调研报告
```

> 说明：`build/`、`.cache/`、`cmake/`、`sdkconfig`、`*.pack`、`local.properties`、`keystore.properties`、`*.jks` 等构建产物与本地配置已通过 `.gitignore` 排除。

## 三端分工

| 维度 | ESP32S3（无线通信端） | GD32H7（实时控制/显示端） | Android APP |
| ---- | --------------------- | -------------------------- | ----------- |
| 定位 | 云端/上位机无线通信、边缘算力、终端直连桥 | 传感器采集、外设驱动、LCD 显示、链路协议 | 现场运维与远程监控 |
| 内核 | Xtensa LX7 双核（240MHz） | Arm Cortex-M7（双核，带 FPU） | Kotlin / Android 8.0+ |
| 链路 | Wi-Fi + MQTT + **BLE GATT(NUS) / TCP 透传桥** | UART/SPI 跨核链路、8080 并口 LCD、蓝牙模块 | MQTT 云链路 + BLE + 经典蓝牙 SPP + LAN TCP |
| 工具链 | ESP-IDF v5.x（idf.py，FreeRTOS） | CMake + arm-none-eabi-gcc（标准外设库） | Gradle 8.6 / AGP 8.4.2 |
| 分层 | main / APP / BSP / System / HAL / Driver / Service | Core / Drivers / Firmware / Startup / Linker | ui / mqtt / ble / bt / net / db / service / util |

### 1. ESP32S3 —— 无线通信端

基于 **ESP-IDF v5.5** 工程结构，负责 Wi-Fi/BLE 接入、云端交互与终端直连透传，通过 FreeRTOS 多任务调度实现业务逻辑。

- 入口：`main/main.c`（初始化表 + 依赖拓扑排序的分阶段初始化框架）
- 云链路：`Service/`（链接协议、事件总线、跨核事件桥、连接 HSM、云端桥接、Wi-Fi/MQTT 管理、凭证管理、A/B OTA）
- **无线透传桥（`Service/net_bridge.c`）**：BLE GATT **Nordic UART Service**（服务 `6E400001-…`，写特征 `6E400002-…`，通知特征 `6E400003-…`）+ **LAN TCP Server（8080）** + mDNS 可选（`MdnsUse`）；透传字节复用跨核 link_protocol 帧，与 UART 主链路无缝互通
- 上行增强：OTA 进度/结果上报（`…/ota`）、命令执行回执（`…/ack`）、status 携带固件版本（`ver`）
- 功能开关：`System/Inc/sys.h`（`BleGattUse` / `TcpSrvUse` / `MdnsUse` 等，组合由 `config_verify.h` 编译期校验）
- 详细说明见 [Industrial_Edge_IoT_Terminal_ESP32S3/README.md](Industrial_Edge_IoT_Terminal_ESP32S3/README.md)

### 2. GD32H7 —— 实时控制/显示端

面向工业现场的实时控制端，负责多种外设驱动、**8080 并口 LCD** 显示、蓝牙（BT24/HC05）链路协议解析，以及双缓冲 DMA 串口接收等实时逻辑。

- 入口：`Core/Src/main.c`
- 板级驱动：`Drivers/BSP/`（LCD、LED、按键、SD 卡、蓝牙、串口、CRC、MPU 等）
- 详细说明见 [Industrial_Edge_IoT_Terminal_GD32H7/README.md](Industrial_Edge_IoT_Terminal_GD32H7/README.md)

### 3. Android APP —— 现场运维终端

Android 原生 Kotlin 实现，覆盖设计文档要求的移动端能力（数据监控、远程配置、OTA 触发、透传监视、BLE 扫描/RSSI 曲线）：

- 四链路：MQTT（监控/控制/OTA，后台保活+自愈重连）、BLE GATT、经典蓝牙 SPP、LAN TCP
- 监控：实时数据、五指标历史曲线、24h 统计、告警历史+通知（勿扰）、SQLite 落盘（30 天清理）+ CSV 导出
- 质量：JVM 单测 + lint + 模拟器冒烟测试（GitHub Actions CI）
- 详细说明见 [Android_APP/README.md](Android_APP/README.md)，真机验证见 [Android_APP/DEVICE_TEST_CHECKLIST.md](Android_APP/DEVICE_TEST_CHECKLIST.md)

## 通信契约（三端协同核心）

- **MQTT 主题**：`ind_edge/terminal/{sensors,alarm,status,cmd,ota,ack}`（载荷格式见契约文档）
- **BLE**：Nordic UART Service（UUID 可在 APP 设置页与固件中配置）
- **SPP / TCP**：标准 SPP UUID / 端口 8080，透传字节统一复用跨核 link 帧
- **跨核契约**：两端镜像 `link_protocol.h` / `link_payload.h` / `event_id.h`
- 完整契约（含 v0.6 草案：OTA 进度、命令 ACK、版本上报）见 `docs/app_firmware_contract.docx`

## 快速开始

### 1. ESP32S3（无线通信端，ESP-IDF v5.x / 已验证 v5.5.4）

```bash
# Windows 激活 IDF 环境（EIM 安装示例）：
#   . C:\Espressif\tools\Microsoft.v5.5.4.PowerShell_profile.ps1
cd Industrial_Edge_IoT_Terminal_ESP32S3
idf.py set-target esp32s3     # 首次
idf.py build
idf.py -p COMx flash monitor
```

> 注：BLE 透传桥依赖 sdkconfig 中 `CONFIG_BT_ENABLED / CONFIG_BT_BLUEDROID_ENABLED / CONFIG_BT_BLE_ENABLED / CONFIG_BT_BLE_42_FEATURES_SUPPORTED`（已写入 `sdkconfig.defaults`）；启用 mDNS 需 `idf.py add-dependency "espressif/mdns"` 并将 `MdnsUse` 置 1。因使用 GLOB 收集源文件，新增/删除源文件后执行 `idf.py fullclean` 再 build。

### 2. GD32H7（实时控制端）

```bash
cd Industrial_Edge_IoT_Terminal_GD32H7
cmake -B build -DCMAKE_TOOLCHAIN_FILE=<arm-none-eabi-toolchain.cmake>
cmake --build build
```

产物为 `build/Industrial_Edge_IoT_Terminal_GD32H7.elf / .bin / .hex`，使用 J-Link / ST-Link 等烧录。

### 3. Android APP

```bash
cd Android_APP
gradlew.bat assembleDebug        # Windows；产物 app/build/outputs/apk/debug/
gradlew.bat assembleRelease      # 需先配置 keystore.properties（模板见目录内）
```

推送到 GitHub 自动触发 CI（构建 + 32 项单测 + lint + 模拟器冒烟），产物含 APK 与 lint 报告 artifact。

## 文档

- 整机总体设计：`docs/smart_iot_terminal_guide.docx`
- APP 与固件通信契约：`docs/app_firmware_contract.docx`
- 双端模块分配调研：`docs/project_modDistribution_survey.docx`
- ESP32S3 端开发指南：`Industrial_Edge_IoT_Terminal_ESP32S3/docs/esp32s3_site_develop_guide.docx`
- GD32H7 端开发指南：`Industrial_Edge_IoT_Terminal_GD32H7/docs/gd32h7_site_develop_guide.docx`
- APP 真机验证清单：`Android_APP/DEVICE_TEST_CHECKLIST.md`

## 许可证

本项目基于 [GPL-3.0](LICENSE) 许可发布。
