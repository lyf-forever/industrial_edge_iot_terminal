# Industrial Edge IoT Terminal — ESP32S3（无线通信端）

本目录为智能终端项目的 **ESP32S3 端**固件，基于乐鑫 **ESP-IDF** 开发，负责 Wi-Fi/BLE 无线接入、云端/上位机通信与应用任务调度。

## 功能概述

- 基于 **FreeRTOS** 的多任务调度（`APP/` 提供任务注册表机制）
- 分阶段初始化框架：硬件初始化 → 软件初始化（`main/main.c` 中的 `initTable`）
- 板级驱动：MQ2 气体浓度传感器（`BSP/`）、WS2812 灯带
- 通过 `System/` 配置宏开关功能，例如 `SensorUse`、`Mq2Use`

## 目录结构

```
Industrial_Edge_IoT_Terminal_ESP32S3/
├── CMakeLists.txt                 # 工程根 CMake（引用 ESP-IDF）
├── sdkconfig                      # ESP-IDF 配置（已被 .gitignore 忽略）
├── main/
│   ├── CMakeLists.txt             # 组件编译配置（GLOB 收集源文件）
│   └── main.c                     # 分阶段初始化 + 任务调度入口
├── APP/                           # 应用层：任务表、业务任务
├── BSP/                           # 板级驱动：mq2、ws2812
├── System/                        # 系统配置与通用工具：sys、self_def
├── HAL/                           # 硬件抽象层（预留）
├── Driver/                        # 设备驱动层（预留）
├── Service/                       # 服务层（预留）
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
| `SensorUse` | 是否启用传感器功能 | 0/1 |
| `Mq2Use`   | 是否启用 MQ2 气体传感器 | 0/1 |
