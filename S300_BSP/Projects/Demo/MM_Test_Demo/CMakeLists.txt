cmake_minimum_required(VERSION 3.16)

project(MM_Test_Demo)

# Target
set(TARGET s300_mm_test_demo)

# Paths
set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ROOT ${PROJECT_DIR}/../../..)
set(DEVICE_DIR ${ROOT}/CMSIS/Device/PiMCHIP/S300)
set(LD_SCRIPT ${PROJECT_DIR}/ld/sram_demo.ld)

# Includes
include_directories(
    ${DEVICE_DIR}/Include
    ${ROOT}/CMSIS/Core/Include
    ${ROOT}/Drivers/SoC/RCC/Include
    ${ROOT}/Drivers/SoC/GPIO/Include
    ${ROOT}/Drivers/SoC/UART/Include
    ${ROOT}/Drivers/SoC/DMA/Include
    ${ROOT}/Drivers/SoC/I2CSoft/Include
    ${ROOT}/Drivers/SoC/MAILBOX/Include
    ${ROOT}/Drivers/External/OV5640/Include
    ${ROOT}/Drivers/SoC/MM/Include
    ${ROOT}/Drivers/SoC/PSRAM/Include
    ${ROOT}/Boards/generic_evb
    ${PROJECT_DIR}/Inc
)

# Sources
set(SRCS
    ${DEVICE_DIR}/Source/GCC/startup_S300.S
    ${ROOT}/Boards/generic_evb/board.c
    ${ROOT}/Boards/Common/retarget.c
    ${ROOT}/Boards/Common/syscalls.c
    ${ROOT}/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c
    ${ROOT}/Drivers/SoC/RCC/Source/rcc.c
    ${ROOT}/Drivers/SoC/GPIO/Source/gpio.c
    ${ROOT}/Drivers/SoC/UART/Source/uart.c
    ${ROOT}/Drivers/SoC/DMA/Source/dma.c
    ${ROOT}/Drivers/SoC/I2CSoft/Source/i2c_soft.c
    ${ROOT}/Drivers/External/OV5640/Source/ov5640.c
    ${PROJECT_DIR}/Src/main.c
    ${PROJECT_DIR}/Src/camera_ov5640.c
    ${PROJECT_DIR}/Src/face_tracker.c
    ${PROJECT_DIR}/Src/uart_cmd.c
    ${PROJECT_DIR}/Src/display_demo_app.c
)

set_source_files_properties(${DEVICE_DIR}/Source/GCC/startup_S300.S PROPERTIES LANGUAGE ASM)

add_executable(${TARGET} ${SRCS})

# Linker script
set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-T${LD_SCRIPT}")

# Link libraries
target_link_libraries(${TARGET}
    s300_video
    s300_mailbox
    s300_psram
    m
)

# Post build artifacts (bin/hex/disasm/size)
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
    COMMAND arm-none-eabi-objdump -d -S -C -M force-thumb,reg-names-std -j .text $<TARGET_FILE:${TARGET}> > ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.dis
    COMMAND arm-none-eabi-size $<TARGET_FILE:${TARGET}>
    COMMENT "Generating binary, hex, disassembly and size info for MM test demo"
)

# Debug target
set(GDB_PORT 3333 CACHE STRING "GDB server port")
add_custom_target(dbg_mm_test
    COMMENT "Launching GDB for ${TARGET} (SRAM: 0x20000000) (GDB_PORT=${GDB_PORT})"
    COMMAND arm-none-eabi-gdb -q
    -ex "file $<TARGET_FILE:${TARGET}>"
    -ex "target extended-remote :${GDB_PORT}"
    -x ${ROOT}/gdbinit.display.gdb
    DEPENDS ${TARGET}
    USES_TERMINAL
)

# Configure GDB script with absolute paths
set(DSP_IMAGES_DIR ${PROJECT_DIR}/DSP_Images)
set(ALGORITHM_MODELS_DIR ${ROOT}/Algorithm_Models)
configure_file(
    ${PROJECT_DIR}/gdbinit.dsp.gdb.in
    ${CMAKE_CURRENT_BINARY_DIR}/gdbinit.dsp.gdb
    @ONLY
)

add_custom_target(dbg_mm_test_dsp
    COMMENT "Launching GDB for ${TARGET} with DSP boot images (SRAM: 0x20000000) (GDB_PORT=${GDB_PORT})"
    COMMAND arm-none-eabi-gdb -q
    -ex "file $<TARGET_FILE:${TARGET}>"
    -ex "target extended-remote :${GDB_PORT}"
    -x ${CMAKE_CURRENT_BINARY_DIR}/gdbinit.dsp.gdb
    DEPENDS ${TARGET}
    USES_TERMINAL
)

# Auto-generate debug targets for each algorithm model
# e.g. Face_Detection -> dbg_mm_test_face-detection
file(GLOB ALG_MODELS RELATIVE ${ALGORITHM_MODELS_DIR} ${ALGORITHM_MODELS_DIR}/*)

foreach(MODEL_DIR ${ALG_MODELS})
    if(IS_DIRECTORY ${ALGORITHM_MODELS_DIR}/${MODEL_DIR})
        # Try to get version info
        execute_process(
            COMMAND python3 ${ROOT}/tools/get_model_version.py ${ALGORITHM_MODELS_DIR}/${MODEL_DIR}
            OUTPUT_VARIABLE MODEL_VER
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if("${MODEL_VER}" STREQUAL "")
            set(MODEL_VER "Unknown")
        endif()

        # Convert Face_Detection to face-detection
        string(TOLOWER ${MODEL_DIR} MODEL_LOWER)
        string(REPLACE "_" "-" MODEL_KEBAB ${MODEL_LOWER})

        add_custom_target(dbg_mm_test_${MODEL_KEBAB}
            COMMENT "Launching GDB for ${TARGET} with Algorithm Model: ${MODEL_DIR} [v${MODEL_VER}]"
            COMMAND ${CMAKE_COMMAND} -E env MOD=${MODEL_DIR} arm-none-eabi-gdb -q
            -ex "file $<TARGET_FILE:${TARGET}>"
            -ex "target extended-remote :${GDB_PORT}"
            -x ${CMAKE_CURRENT_BINARY_DIR}/gdbinit.dsp.gdb
            DEPENDS ${TARGET}
            USES_TERMINAL
        )
    endif()
endforeach()
