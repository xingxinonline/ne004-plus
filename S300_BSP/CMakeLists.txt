cmake_minimum_required(VERSION 3.16)

# Set the toolchain for ARM Cortex-M4
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
set(CMAKE_C_COMPILER /home/xinhao/crosstools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER /home/xinhao/crosstools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER /home/xinhao/crosstools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc)

# Since it's bare-metal, don't try to link executables during compiler tests
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set the project name
project(S300_BSP)

# Set compiler flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Os -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra -g3 -gdwarf-2")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS}")

# Enable ASM language
enable_language(ASM)

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles -Wl,--gc-sections -specs=nosys.specs")

# Add subdirectories
add_subdirectory(CMSIS)
add_subdirectory(Drivers)
add_subdirectory(Projects)

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()