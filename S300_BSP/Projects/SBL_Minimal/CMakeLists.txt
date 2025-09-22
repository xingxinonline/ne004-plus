cmake_minimum_required(VERSION 3.16)

project(SBL_Minimal)

# Set the target name
set(TARGET s300_sbl_minimal)

# Set the build directory
set(BUILD_DIR ${CMAKE_BINARY_DIR})

# Set paths
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ROOT ${PROJECT_DIR}/../..)
set(DEVICE_DIR ${ROOT}/CMSIS/Device/PiMCHIP/S300)
set(LD_SCRIPT ${PROJECT_DIR}/ld/flash.ld)

# Include directories
include_directories(
    ${DEVICE_DIR}/Include
    ${ROOT}/CMSIS/Core/Include
    ${PROJECT_DIR}/Inc
    ${ROOT}/Drivers/SoC/RCC/Include
)

# Compiler definitions
add_definitions(-DSBL_BUILD=1)

# Source files
set(SRCS
    ${DEVICE_DIR}/Source/GCC/startup_S300.S
    ${DEVICE_DIR}/Source/system_S300.c
    ${PROJECT_DIR}/Src/sbl_minimal.c
)

# Set language for assembly files
set_source_files_properties(${DEVICE_DIR}/Source/GCC/startup_S300.S PROPERTIES LANGUAGE ASM)

# Create executable
add_executable(${TARGET} ${SRCS})

# Set linker script
set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-T${LD_SCRIPT}")

# Custom commands for post-build
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
    COMMAND arm-none-eabi-objcopy -O ihex ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
    COMMAND arm-none-eabi-objdump -d -S -C -M force-thumb,reg-names-std -j .text ${CMAKE_CURRENT_BINARY_DIR}/${TARGET} > ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.dis
    COMMAND arm-none-eabi-size ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}
    COMMENT "Generating binary, hex, disassembly and size info"
)