# Industrial Edge IoT Terminal — ESP32S3（无线通信端）

本目录为智能终端项目的 **ESP32S3 端**固件，基于乐鑫 **ESP-IDF** 开发，负责 Wi-Fi/BLE 无线接入、云端/上位机通信与应用任务调度。

## 功能概述

- 基于 **FreeRTOS** 的多任务调度（`APP/` 提供任务注册表机制）
- 分阶段初始化框架：硬件初始化 → 软件初始化（`main/main.c` 中的 `initTable`）
- 板级驱动：MQ2 气体浓度传感器（`BSP/`）、WS2812 灯带、UART 串口链路（`BSP/bsp_uart`）
- 串口链路协议（`Service/link_protocol`）：与 GD32H7 端共享的帧格式，含 CRC16-Modbus + 字节转义
- 链路载荷语义（`Service/link_payload`）：定义 `data[12]` 的传感器数据 overlay，**两端共享**
- Wi-Fi 连接管理（`Service/wifi_manager`）：STA 模式 + 断线自动重连
- MQTT 云端客户端（`Service/mqtt_client_app`）：与工业 IoT 平台对接
- **事件总线（`Service/event_bus`）**：系统级消息中枢，发布/订阅解耦，跨核透明
- **跨核事件桥接（`Service/event_ipc`）**：本地事件 ⇄ link 帧互转，实现"跨核透明"分发
- **连接层次状态机（`Service/conn_hsm`）**：HSM 管理全链路连接状态，驱动 WS2812 状态灯
- 云端桥接服务（`Service/cloud_bridge`）：基于事件总线，传感器/告警⇄MQTT JSON 路由
- **OTA 下载闭环（`Service/ota_downloader` + `ota_mgr`）**：esp_http_client 下载 → 校验 → A/B 分区切换 → 回滚（分区表见 `partitions_ota.csv`）
- **HS-SPI 双链路冗余（`Service/channel_spi` + `channel`）**：UART + SPI 双通道，心跳超时热切换
- **安全凭证管理（`Service/cred_mgr`）**：NVS 集中管理 Wi-Fi/MQTT/TLS 凭证，wifi/mqtt 启动自动读取
- **主题化订阅（`Service/topic`）**：分层主题 + 前缀通配，接入跨核事件流
- **宏组合编译期校验（`System/Inc/config_verify.h`）**：功能开关依赖一致性 #error 校验
- 通过 `System/` 配置宏开关功能，例如 `SensorUse`、`Mq2Use`、`LinkUse`、`WiFiUse`、`MqttUse`、`CloudUse`、`EventBusUse`、`HsmUse`、`Ws2812Use`、`OtaUse`、`ChannelSpiUse` 等

## 软件架构（与整机设计对齐）

整机设计文档将"事件总线跨核化"作为系统级消息中枢：模块订阅/发布事件时无需关心事件由哪颗芯片处理。本端实现：

- **event_bus**：本地发布订阅中枢（订阅者数组 + 同步分发，线程安全）
- **event_ipc**：既是总线通配订阅者（TX：本地事件→link帧），又是 link 帧接收者（RX：link事件帧→本地总线），实现跨核透明
- **conn_hsm**：订阅 `EVT_COMM_*` 状态事件，层次状态机管理 OFFLINE / CLOUD_DISCONNECTED / CLOUD_CONNECTED，跃迁时驱动 WS2812 灯色
- **cloud_bridge**：订阅传感器/告警/心跳事件→MQTT JSON 上报；MQTT 下行命令→发布控制事件（自动跨核转发至 GD32H7）
- 业务模块（如 SensorTask）只需 `event_bus_publish`，下游由总线分发，不再硬编码调用关系

## 数据流（事件总线版）

```
上行: 采集/对端 --> event_bus.publish(EVT_SENSOR_DATA, ANY)
        ├── [本地] cloud_bridge 订阅 --> JSON --> mqtt_client_app --> 云平台
        └── [跨核] event_ipc 通配订阅 --> link 帧 --> bsp_uart --> GD32H7 总线
下行: 云平台 --> mqtt_client_app --> cloud_bridge --> event_bus.publish(EVT_CTRL_LED, ANY)
        └── event_ipc --> link 帧 --> GD32H7 执行
状态: wifi/mqtt/link 状态 --> event_bus(EVT_COMM_*) --> conn_hsm(跃迁) --> WS2812 灯色
```

## 目录结构

```
Industrial_Edge_IoT_Terminal_ESP32S3/
├── CMakeLists.txt                 # 工程根 CMake（引用 ESP-IDF）
├── sdkconfig                      # ESP-IDF 配置（已被 .gitignore 忽略）
├── main/
│   ├── CMakeLists.txt             # 组件编译配置（GLOB 收集源文件）
│   └── main.c                     # 分阶段初始化 + 任务调度入口
├── APP/                           # 应用层：任务表、SensorTask
├── BSP/                           # 板级驱动：bsp_mq2、bsp_ws2812、bsp_uart(串口链路)
├── System/                        # 系统配置与通用工具：sys、self_def
├── HAL/                           # 硬件抽象层（预留）
├── Driver/                        # 设备驱动层（预留）
├── Service/                       # 服务层：link_protocol、link_payload、event_id、
│                                  #         event_bus、event_ipc、conn_hsm、cloud_bridge、
│                                  #         wifi_manager、mqtt_client_app
├── docs/
│   └── esp32s3_site_develop_guide.docx
├── .devcontainer/                 # 容器化开发环境
├── .vscode/                       # VSCode 调试/编译配置
└── .clangd                        # clangd 配置
```

## 环境与编译

1. 安装 **ESP-IDF v5.x** 并配置好环境变量（`idf.py` 可用）。
2. 设置目标芯片并编译：

   ```bash
   idf.py set-target esp32s3
   idf.py build
   idf.py -p COMx flash monitor
   ```

3. `main/CMakeLists.txt` 使用 `file(GLOB ...)` 自动收集源文件，**新增/删除源文件后需重新执行 `idf.py fullclean` 再 build**。

## 功能开关（`System/Inc/sys.h`）

| 宏 | 说明 | 取值 |
| -- | ---- | ---- |
| `SensorUse`  | 是否启用传感器功能 | 0/1 |
| `Mq2Use`    | 是否启用 MQ2 气体传感器 | 0/1 |
| `Ws2812Use` | 是否启用 WS2812 状态指示灯带 | 0/1 |
| `LinkUse`   | 是否启用与 GD32H7 的串口链路（复用 link_protocol） | 0/1 |
| `WiFiUse`   | 是否启用 Wi-Fi 站点连接 | 0/1 |
| `MqttUse`   | 是否启用 MQTT 云端客户端（依赖 WiFiUse） | 0/1 |
| `CloudUse`    | 是否启用云端桥接服务（依赖 LinkUse 与 MqttUse） | 0/1 |
| `EventBusUse` | 是否启用事件总线（系统级消息中枢） | 0/1 |
| `HsmUse`     | 是否启用连接层次状态机 | 0/1 |
| `ModRegUse`  | 架构3.1 模块注册表与依赖拓扑排序 | 0/1 |
| `MemPoolUse` | 架构3.6 内存池/对象池 | 0/1 |
| `SoftTimerUse` | 架构3.8 软定时器轮 | 0/1 |
| `ChannelUse` | 架构3.3 通道抽象 | 0/1 |
| `ChannelSpiUse` | 架构3.3b HS-SPI 真实通道（0=占位 stub） | 0/1 |
| `CmdUse`     | 架构3.7 命令分发器 | 0/1 |
| `HsmFwUse`   | 架构3.10 通用 HSM 框架 | 0/1 |
| `LogUse`     | 架构3.9 结构化日志 | 0/1 |
| `TtsUse`     | 架构3.5 时间触发调度器 | 0/1 |
| `ActorUse`   | 架构3.4 Actor 模型 | 0/1 |
| `TopicUse`   | 架构3.2 主题化订阅 | 0/1 |
| `CredUse`    | 架构3.12 凭证管理器 | 0/1 |
| `OtaUse`     | 架构3.11 A/B 分区 OTA | 0/1 |
| `AiUse`      | 架构3.13 边缘 AI 推理管线 | 0/1 |

> 修改功能开关后需在 `sys.h` 中调整，并通过 `initTable` / `taskTable` 条件编译自动裁剪对应模块。
> 依赖组合由 `System/Inc/config_verify.h` 编译期校验（如 MqttUse=1 必须 WiFiUse=1）。

## 跨核共享契约（两端正协同）

`Service/Inc/event_id.h` 与 `Service/Inc/link_payload.h` 是双 MCU 共享的"事件契约"，
GD32H7 端需镜像相同的事件 ID 枚举与载荷 overlay，方可实现跨核事件总线透明分发。
新增事件 ID 时务必两端同步更新。
