# Industrial Edge IoT Terminal — GD32H7（实时控制/显示端）

本目录为智能终端项目的 **GD32H7 端**固件，基于 **GD32H7 系列 Cortex-M7** 双核 MCU，负责现场数据采集、外设驱动、**8080 并口 LCD** 显示及蓝牙链路协议处理等实时控制逻辑。

## 功能概述

- 基于 **GD32H7xx 标准外设库**（STD_Peripheral）与 CMSIS
- 多种板级外设：8080 并口 LCD、LED、按键、SD 卡（SDRAM）、蓝牙（BT24 / HC05）、串口、CRC、MPU
- 双缓冲 **DMA 串口接收** + 链路协议解析（`link_protocol`，含字节转义解码与帧回调）
- 环形缓冲日志系统（`log_ringbuf`，头部与 ESP32S3 端 log_fw 兼容）
- **事件总线 + 跨核事件桥接**（`SYSTEM/event_bus` / `event_ipc`）：与 ESP32S3 共享事件契约，跨核透明分发
- **软定时器回调表**（`driver_tick_handle`，SysTick 驱动 1/10/100ms 三轮）
- **双池内存分配器**（内部 SRAM + 外部 SDRAM，`malloc.c`）
- **HS-SPI 从机链路**（`BSP/spi_slave_link`）：与 ESP32S3 SPI master 构成双链路冗余
- 工程由 **CMake + arm-none-eabi-gcc** 构建

## 目录结构

```
Industrial_Edge_IoT_Terminal_GD32H7/
├── CMakeLists.txt                 # CMake 构建脚本（C + ASM）
├── Core/                          # 用户核心代码
│   ├── Inc/  main.h / systick.h / gd32h7xx_it.h ...
│   └── Src/  main.c / systick.c / gd32h7xx_it.c ...
├── Drivers/
│   ├── BSP/                       # 板级驱动：LCD、LED、按键、蓝牙、串口、SD、CRC、MPU ...
│   ├── MCU/                       # MCU 外设驱动：GPIO、DMA、UART、SPI、I2C、ADC、EXMC、Timer ...
│   └── SYSTEM/                    # 系统级：日志、内存管理（malloc）、公共工具
├── Firmware/
│   ├── CMSIS/                     # 内核与 GD32H7xx 启动/系统文件
│   ├── STD_Peripheral/            # GD32H7xx 标准外设库
│   └── USBHS_Lib/                 # USB 高速库（预留）
├── Startup/
│   └── startup_gd32h7xx.S         # 启动文件
├── Linker/
│   └── gd32h7xx_flash.ld          # 链接脚本
├── docs/
│   └── gd32h7_site_develop_guide.docx
├── syscalls.c                     # 新库 syscall 适配
└── .vscode/                       # VSCode 调试/编译配置
```

## 环境与编译

1. 准备 **CMake（≥3.12）** 与 **arm-none-eabi-gcc** 工具链（支持 Cortex-M7 + FPv5）。
2. 配置并编译：

   ```bash
   cmake -B build -DCMAKE_TOOLCHAIN_FILE=<path-to-arm-none-eabi-gcc-toolchain.cmake>
   cmake --build build
   ```

3. 产物输出至 `build/`：

   - `Industrial_Edge_IoT_Terminal_GD32H7.elf`
   - `Industrial_Edge_IoT_Terminal_GD32H7.bin`
   - `Industrial_Edge_IoT_Terminal_GD32H7.hex`

4. 使用 J-Link / ST-Link / DAP-Link 等工具将 `.hex` 或 `.bin` 烧录至芯片。

## 编译关键配置（`CMakeLists.txt`）

| 项 | 值 |
| -- | -- |
| CPU | `-mcpu=cortex-m7 -mthumb` |
| FPU | `-mfpu=fpv5-d16 -mfloat-abi=hard` |
| 链接脚本 | `Linker/gd32h7xx_flash.ld` |
| 宏定义 | `GD32H7XX`、`USE_STDPERIPH_DRIVER`、`__FPU_PRESENT=1` |
