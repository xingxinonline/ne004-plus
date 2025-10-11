cmake_minimum_required(VERSION 3.16)

project(RBL_Minimal)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Set the target name
set(TARGET s300_rbl_minimal)

# Set the build directory
set(BUILD_DIR ${CMAKE_BINARY_DIR})

# Set paths
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ROOT ${PROJECT_DIR}/../..)
set(DEVICE_DIR ${ROOT}/CMSIS/Device/PiMCHIP/S300)
set(LD_SCRIPT ${PROJECT_DIR}/ld/sram64.ld)

set(S300_TOOLS_DIR ${PROJECT_DIR}/tools)
set(S300_GENERATE_HEADER_PY ${S300_TOOLS_DIR}/generate_s300_standard_header.py)
set(S300_MERGE_IMAGE_PY ${S300_TOOLS_DIR}/merge_s300_image.py)
set(S300_ANALYZE_HEADER_PY ${S300_TOOLS_DIR}/analyze_s300_header.py)

set(S300_IMAGE_DEST_DIR "/mnt/g/workspace/next/bin" CACHE PATH "Destination directory for S300 complete image")

set(S300_TARGET_BIN ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin)
set(S300_TARGET_HEADER ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_header.bin)
set(S300_TARGET_COMPLETE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_complete.bin)
set(S300_TARGET_COMPLETE_DEST ${S300_IMAGE_DEST_DIR}/${TARGET}_complete.bin)

set(S300_FLASH_ADDRESS 0x08000000 CACHE STRING "Flash base address for S300 RBL image")
set(S300_GDB_CLIENT arm-none-eabi-gdb CACHE STRING "GDB client executable for flashing")
file(TO_CMAKE_PATH "${S300_TARGET_COMPLETE}" S300_TARGET_COMPLETE_FOR_GDB)

# Include directories
include_directories(
    ${DEVICE_DIR}/Include
    ${ROOT}/CMSIS/Core/Include
    ${PROJECT_DIR}/Inc
    ${ROOT}/Drivers/SoC/QSPI/Include
    ${ROOT}/Drivers/SoC/RCC/Include
)

# Compiler definitions
add_definitions(-DRBL_LOG_ENABLE=1)
add_definitions(-Dprintf=rbl_printf_stub)

# Source files
set(SRCS
    ${DEVICE_DIR}/Source/GCC/startup_S300.S
    ${PROJECT_DIR}/Src/rbl_hal.c
    ${ROOT}/Drivers/SoC/RCC/Source/rcc.c
    ${ROOT}/Drivers/SoC/QSPI/Source/qspi_cadence.c
    ${PROJECT_DIR}/Src/rbl_qspi.c
    ${PROJECT_DIR}/Src/rbl_sbl.c
    ${PROJECT_DIR}/Src/rbl_fault.c
    ${PROJECT_DIR}/Src/rbl_simple.c
)

# Set language for assembly files
set_source_files_properties(${DEVICE_DIR}/Source/GCC/startup_S300.S PROPERTIES LANGUAGE ASM)

# Create executable
add_executable(${TARGET} ${SRCS})

# Set linker script
set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-T${LD_SCRIPT}")

# Custom commands for post-build
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${TARGET}> ${S300_TARGET_BIN}
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
    COMMAND arm-none-eabi-objdump -d -S -C -M force-thumb,reg-names-std -j .text $<TARGET_FILE:${TARGET}> > ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.dis
    COMMAND arm-none-eabi-size $<TARGET_FILE:${TARGET}>
    COMMENT "Generating binary, hex, disassembly and size info"
)

add_custom_command(
    OUTPUT ${S300_TARGET_HEADER}
    COMMAND ${Python3_EXECUTABLE} ${S300_GENERATE_HEADER_PY} generate ${S300_TARGET_BIN} ${S300_TARGET_HEADER}
    DEPENDS ${TARGET} ${S300_GENERATE_HEADER_PY}
    COMMENT "Generating S300 header image"
    VERBATIM
)

add_custom_command(
    OUTPUT ${S300_TARGET_COMPLETE}
    COMMAND ${Python3_EXECUTABLE} ${S300_MERGE_IMAGE_PY} ${S300_TARGET_HEADER} ${S300_TARGET_BIN} ${S300_TARGET_COMPLETE}
    DEPENDS ${S300_TARGET_HEADER} ${S300_MERGE_IMAGE_PY}
    COMMENT "Merging S300 header and payload into complete image"
    VERBATIM
)

add_custom_command(
    OUTPUT ${S300_TARGET_COMPLETE_DEST}
    COMMAND ${CMAKE_COMMAND} -E echo "S300 complete image: ${S300_TARGET_COMPLETE}"
    COMMAND ${CMAKE_COMMAND} -E make_directory ${S300_IMAGE_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${S300_TARGET_COMPLETE} ${S300_TARGET_COMPLETE_DEST}
    COMMAND ${CMAKE_COMMAND} -E echo "Copied to ${S300_TARGET_COMPLETE_DEST}"
    DEPENDS ${S300_TARGET_COMPLETE}
    COMMENT "Copying S300 complete image to ${S300_IMAGE_DEST_DIR}"
    VERBATIM
)

add_custom_target(s300_image
    DEPENDS ${S300_TARGET_COMPLETE_DEST}
)

add_custom_target(analyze_header
    DEPENDS ${S300_TARGET_HEADER}
    COMMAND ${Python3_EXECUTABLE} ${S300_ANALYZE_HEADER_PY} ${S300_TARGET_HEADER}
    COMMENT "Analyzing S300 header format"
    VERBATIM
)

add_custom_target(flash_image
    DEPENDS ${S300_TARGET_COMPLETE}
    COMMAND ${CMAKE_COMMAND} -E echo "Flashing S300 image to ${S300_FLASH_ADDRESS} via ${S300_GDB_CLIENT}"
    COMMAND ${S300_GDB_CLIENT}
    -q
    -ex "target extended-remote :3333"
    -ex "monitor targets ne005.m4"
    -ex "monitor halt"
    -ex "monitor soft_reset_halt"
    -ex "monitor wait_halt 2000"
    -ex "restore '${S300_TARGET_COMPLETE_FOR_GDB}' binary ${S300_FLASH_ADDRESS}"
    -ex "monitor reset run"
    -ex "quit"
    COMMENT "Flashing complete S300 image to ${S300_FLASH_ADDRESS}"
    VERBATIM
)
