# 裸机交叉编译工具链（arm-none-eabi-gcc）
# 用法： cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
#        cmake --build build
# 亦可配合任意 generator（Unix Makefiles / Ninja / Ninja Multi-Config）。

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# 试编译不做链接（避免 CMake 用宿主链接参数）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(ARM_GCC arm-none-eabi-gcc REQUIRED)
set(CMAKE_C_COMPILER ${ARM_GCC})
set(CMAKE_ASM_COMPILER ${ARM_GCC})

find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_OBJDUMP arm-none-eabi-objdump)
find_program(ARM_SIZE arm-none-eabi-size)
find_program(ARM_GDB arm-none-eabi-gdb)
