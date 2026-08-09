# CMake 最低版本要求
cmake_minimum_required(VERSION 3.12)

# 项目名称，支持 C 和 ASM
project(Industrial_Edge_IoT_Terminal_GD32H7 C ASM)

# ==================== 全局编译与链接选项 ====================
# 设置通用的编译标志
set(COMMON_FLAGS 
    -mcpu=cortex-m7        # 指定 CPU 为 Cortex-M7
    -mthumb                # 生成 Thumb 指令集代码
    -mfpu=fpv5-d16         # 启用双精度硬件 FPU (FPv5)
    -mfloat-abi=hard       # 使用硬件浮点调用约定
    -fstack-usage          # 生成栈使用信息
    -Wall                  # 开启常用警告
    -Wextra                # 开启更多警告
    -Werror                # 将警告视为错误，保证代码质量
    -Wno-unused-parameter  # 忽略未使用参数的警告
    -g3                    # 生成丰富的调试信息
    -O0                    # 关闭优化，便于调试
    -ffunction-sections    # 将每个函数放入独立段，便于链接时优化
    -fdata-sections        # 将每个数据放入独立段
    -mlong-calls                   
)

enable_language(ASM)  # 启用汇编语言支持

# 设置编译选项
add_compile_options(${COMMON_FLAGS})

# # 为汇编语言启用预处理功能（如果启动文件是 .S 后缀）
# add_compile_options(
#     $<$<COMPILE_LANGUAGE:ASM>:-x assembler-with-cpp>
# )

# ==================== 链接选项 ====================
# 设置链接脚本路径，请替换为你的实际路径
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/Linker/gd32h7xx_flash.ld)

# 设置链接选项
add_link_options(
    ${COMMON_FLAGS}                # 复用通用的 CPU 和 FPU 选项
    -T ${LINKER_SCRIPT}            # 指定链接脚本
    -Wl,--no-relax                 # 禁止链接器优化，避免地址对齐问题    
    -Wl,--gc-sections              # 移除未使用的段
    -Wl,-Map=${PROJECT_NAME}.map   # 生成内存映射文件，便于分析
    -specs=nano.specs              # 使用 Newlib-nano 库，减小代码体积
    -specs=nosys.specs             # 禁止半主机模式，避免程序卡死
    
)

# ==================== 宏定义 ====================
# 根据芯片系列和使用的库进行定义
add_compile_definitions(
    GD32H7XX                       # 芯片型号宏
    __FPU_PRESENT=1                # 告知启动文件 FPU 存在
    USE_STDPERIPH_DRIVER           # 使用标准外设库
    # USE_USB_FS                     # 使用USBHS
    # USE_USBHS0                     # 使用USBHS0
    # USE_HAL_DRIVER               # 如果使用 HAL 库，请注释上一行并取消注释这一行
)

# ==================== 收集源文件 ====================
# 1. 用户应用代码 (Core/Src)
file(GLOB_RECURSE APP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.c)

# 2. 固件库源码
# 请根据你实际存放标准外设库或 HAL 库的路径进行修改
set(FIRMWARE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Firmware)

# 标准外设库路径 
file(GLOB_RECURSE LIB_SOURCES
    ${FIRMWARE_DIR}/STD_Peripheral/Source/*.c
    ${FIRMWARE_DIR}/CMSIS/GD32H7xx/Source/*.c
)

# BSP用户驱动库路径 
file(GLOB_RECURSE BSP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Src/*.c)

# 系统级驱动库路径 
file(GLOB_RECURSE SYS_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/SYSTEM/Src/*.c)

# MCU驱动库路径 
file(GLOB_RECURSE MCUDRIVER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU/Src/*.c)

# # USB官方驱动库路径 
# file(GLOB_RECURSE USB_SOURCES 
#     ${FIRMWARE_DIR}/USBHS_Lib/device/class/cdc/Source/*.c
#     ${FIRMWARE_DIR}/USBHS_Lib/device/core/Source/*.c
#     ${FIRMWARE_DIR}/USBHS_Lib/driver/Source/drv_usb_dev.c
#     ${FIRMWARE_DIR}/USBHS_Lib/driver/Source/drv_usb_core.c
#     ${FIRMWARE_DIR}/USBHS_Lib/driver/Source/drv_usbd_int.c
#     # ${FIRMWARE_DIR}/USBHS_Lib/host/class/cdc/Source/*.c
#     # ${FIRMWARE_DIR}/USBHS_Lib/host/class/hid/Source/*.c
#     # ${FIRMWARE_DIR}/USBHS_Lib/host/class/msc/Source/*.c
#     # ${FIRMWARE_DIR}/USBHS_Lib/host/core/Source/*.c
# )

# 3. 启动文件 (请确认你的启动文件后缀，如 .s 或 .S)
set(STARTUP_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Startup/startup_gd32h7xx.S)

# 汇总所有源文件
set(ALL_SOURCES
    ${STARTUP_FILE}
    ${APP_SOURCES}
    ${LIB_SOURCES}
    ${BSP_SOURCES}
    ${SYS_SOURCES}
    ${MCUDRIVER_SOURCES}
   # ${USB_SOURCES}
)

# ==================== 生成可执行文件 ====================
add_executable(${PROJECT_NAME}.elf ${ALL_SOURCES})

# ==================== 生成 .bin 和 .hex 文件 ====================
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    COMMAND arm-none-eabi-objcopy -O ihex ${PROJECT_NAME}.elf ${PROJECT_NAME}.hex
    COMMENT "Generating binary and hex files"
)

# ==================== 头文件路径 ====================
target_include_directories(${PROJECT_NAME}.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc                # 用户头文件目录
    ${FIRMWARE_DIR}/CMSIS                               # CMSIS 核心头文件
    ${FIRMWARE_DIR}/CMSIS/GD32H7xx/Include              # 标准 CMSIS 头文件路径
    ${FIRMWARE_DIR}/STD_Peripheral/Include              # 标准外设库头文件路径
    # ${FIRMWARE_DIR}/USBHS_Lib/driver/Include            # USB官方库头文件路径-头
    # ${FIRMWARE_DIR}/USBHS_Lib/device/class/cdc/Include  
    # ${FIRMWARE_DIR}/USBHS_Lib/device/core/Include
    # #${FIRMWARE_DIR}/USBHS_Lib/host/class/cdc/Include
    # #${FIRMWARE_DIR}/USBHS_Lib/host/class/hid/Include
    # #${FIRMWARE_DIR}/USBHS_Lib/host/class/msc/Include
    # #${FIRMWARE_DIR}/USBHS_Lib/host/core/Include         
    # ${FIRMWARE_DIR}/USBHS_Lib/ustd/class/cdc
    # # ${FIRMWARE_DIR}/USBHS_Lib/ustd/class/hid
    # # ${FIRMWARE_DIR}/USBHS_Lib/ustd/class/msc
    # ${FIRMWARE_DIR}/USBHS_Lib/ustd/common               # USB官方库头文件路径-尾
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/BSP/Inc         # 用户驱动库头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/System/Inc      # 系统级驱动库头文件路径
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/MCU/Inc         # MCU驱动库头文件路径
)

# ==================== 指定最终输出文件目录（可选） ====================
set_target_properties(${PROJECT_NAME}.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)






