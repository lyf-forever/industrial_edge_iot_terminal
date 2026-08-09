# industrial_edge_iot_terminal

基于 **双 MCU** 架构的工业级边缘物联网智能终端（Industrial Edge IoT Terminal）。本仓库同时维护 **ESP32S3** 与 **GD32H7** 两端 MCU 的完整固件源码，两端分工明确、目录独立，可分别编译、烧录与调试。

## 目录结构（快速区分两端 MCU）

```
industrial_edge_iot_terminal/
├── README.md                                    # 本文件：仓库总览
├── LICENSE                                      # GPL-3.0 开源许可
├── Industrial_Edge_IoT_Terminal_ESP32S3/        # 【端 1】Wi-Fi/BLE 无线通信端
├── Industrial_Edge_IoT_Terminal_GD32H7/         # 【端 2】实时采集/显示控制端
└── docs/
    └── smart_iot_terminal_guide.docx            # 整机总体设计说明文档
```

> 说明：`build/`、`.cache/`、`cmake/`、`sdkconfig`、`*.pack` 等构建产物与缓存已通过 `.gitignore` 排除，不进入版本库。

## 两端 MCU 分工

| 维度 | ESP32S3（无线通信端） | GD32H7（实时控制/显示端） |
| ---- | --------------------- | -------------------------- |
| 定位 | 云端/上位机无线通信、应用任务调度 | 传感器采集、外设驱动、LCD 显示、链路协议 |
| 内核 | Xtensa LX7 双核（240MHz） | Arm Cortex-M7（双核，带 FPU） |
| 工具链 | ESP-IDF（idf.py，FreeRTOS） | CMake + arm-none-eabi-gcc（标准外设库） |
| 目录 | `Industrial_Edge_IoT_Terminal_ESP32S3/` | `Industrial_Edge_IoT_Terminal_GD32H7/` |
| 开发文档 | `docs/esp32s3_site_develop_guide.docx` | `docs/gd32h7_site_develop_guide.docx` |
| 分层 | main / APP / BSP / System / HAL / Driver / Service | Core / Drivers / Firmware / Startup / Linker |

### 1. ESP32S3 —— 无线通信端

基于乐鑫 **ESP-IDF** 工程结构，负责 Wi-Fi/BLE 接入与云端交互，内部按 `APP`（任务表）、`BSP`（板级驱动）、`System`（系统配置）分层，并通过 FreeRTOS 多任务调度实现业务逻辑。

- 入口：`main/main.c`（基于初始化表 + 任务注册表的分阶段初始化框架）
- 板级驱动：`BSP/`（MQ2 气体传感器、WS2812 灯带等）
- 系统配置：`System/`（功能开关宏，如 `SensorUse` / `Mq2Use`）
- 详细说明见 [Industrial_Edge_IoT_Terminal_ESP32S3/README.md](Industrial_Edge_IoT_Terminal_ESP32S3/README.md)

### 2. GD32H7 —— 实时控制/显示端

面向工业现场的实时控制端，负责多种外设驱动、**8080 并口 LCD** 显示、蓝牙（BT24/HC05）链路协议解析，以及双缓冲 DMA 串口接收等实时逻辑。

- 入口：`Core/Src/main.c`
- 板级驱动：`Drivers/BSP/`（LCD、LED、按键、SD 卡、蓝牙、串口、CRC、MPU 等）
- MCU 外设驱动：`Drivers/MCU/`（GPIO、DMA、UART、SPI、I2C、ADC、EXMC 等）
- 固件库：`Firmware/`（CMSIS、GD32H7xx 标准外设库、USBHS 库）
- 编译：CMake + `arm-none-eabi-gcc`（链接脚本 `Linker/gd32h7xx_flash.ld`）
- 详细说明见 [Industrial_Edge_IoT_Terminal_GD32H7/README.md](Industrial_Edge_IoT_Terminal_GD32H7/README.md)

## 快速开始

1. **克隆仓库**

   ```bash
   git clone https://github.com/lyf-forever/industrial_edge_iot_terminal.git
   ```

2. **ESP32S3（无线通信端）**

   ```bash
   cd Industrial_Edge_IoT_Terminal_ESP32S3
   idf.py set-target esp32s3
   idf.py build
   idf.py -p COMx flash monitor
   ```

   前提：已安装 ESP-IDF 开发环境（v5.x），并执行 `idf.py fullclean` 以更新 GLOB 收集的源文件列表。

3. **GD32H7（实时控制端）**

   ```bash
   cd Industrial_Edge_IoT_Terminal_GD32H7
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=<arm-none-eabi-toolchain.cmake>
   cmake --build build
   ```

   产物为 `build/Industrial_Edge_IoT_Terminal_GD32H7.elf / .bin / .hex`，使用 J-Link / ST-Link 等烧录。

## 文档

- 整机总体说明：`docs/smart_iot_terminal_guide.docx`
- ESP32S3 端开发指南：`Industrial_Edge_IoT_Terminal_ESP32S3/docs/esp32s3_site_develop_guide.docx`
- GD32H7 端开发指南：`Industrial_Edge_IoT_Terminal_GD32H7/docs/gd32h7_site_develop_guide.docx`

## 许可证

本项目基于 [GPL-3.0](LICENSE) 许可发布。
