cmake_minimum_required(VERSION 3.16)

project(LVGL_Display_Demo)

# Target
set(TARGET s300_lvgl_demo)

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
    ${ROOT}/Drivers/SoC/MM/Include
    ${ROOT}/Drivers/SoC/PSRAM/Include
    ${ROOT}/Drivers/External/OV5640/Include
    ${ROOT}/Boards/generic_evb
    ${PROJECT_DIR}/Inc
)

# Integrate LVGL v9.4 (online via FetchContent or offline via local path)
# Option: provide a local LVGL source tree to avoid network access
set(LVGL_LOCAL_PATH "" CACHE PATH "Local path to LVGL sources (if set, FetchContent is skipped)")
set(LVGL_VENDOR_DIR ${PROJECT_DIR}/third_party/lvgl/lvgl)

# Point LVGL to our local config header
set(LV_CONF_PATH ${PROJECT_DIR}/third_party/lvgl/lv_conf.h CACHE STRING "Path to lv_conf.h" FORCE)

# Configure LVGL options to keep footprint small on MCU (must be set before adding lvgl)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "Build LVGL examples" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "Build LVGL demos" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "Use ThorVG internal" FORCE)
set(LV_BUILD_CONF_PATH ${LV_CONF_PATH} CACHE PATH "Path to lv_conf.h file" FORCE)

if(EXISTS ${LVGL_VENDOR_DIR}/CMakeLists.txt AND EXISTS ${LVGL_VENDOR_DIR}/lvgl.h)
    message(STATUS "Using LVGL from vendor directory: ${LVGL_VENDOR_DIR}")
    add_subdirectory(${LVGL_VENDOR_DIR} ${CMAKE_CURRENT_BINARY_DIR}/lvgl-vendor-build)
elseif(LVGL_LOCAL_PATH)
    message(STATUS "Using LVGL from local path: ${LVGL_LOCAL_PATH}")
    add_subdirectory(${LVGL_LOCAL_PATH} ${CMAKE_CURRENT_BINARY_DIR}/lvgl-local-build)
else()
    include(FetchContent)
    FetchContent_Declare(
        lvgl
        GIT_REPOSITORY https://github.com/lvgl/lvgl.git
        GIT_TAG v9.4.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(lvgl)
endif()

# Ensure lvgl sees LV_CONF_PATH
# if(TARGET lvgl)
# target_compile_definitions(lvgl PUBLIC LV_CONF_PATH="${LV_CONF_PATH}")
# endif()

# Sources
set(SRCS
    ${DEVICE_DIR}/Source/GCC/startup_S300.S
    ${ROOT}/Boards/generic_evb/board.c
    ${ROOT}/Boards/generic_evb/retarget.c
    ${ROOT}/Boards/generic_evb/syscalls.c
    ${ROOT}/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c
    ${ROOT}/Drivers/SoC/RCC/Source/rcc.c
    ${ROOT}/Drivers/SoC/GPIO/Source/gpio.c
    ${ROOT}/Drivers/SoC/UART/Source/uart.c
    ${ROOT}/Drivers/SoC/DMA/Source/dma.c
    ${ROOT}/Drivers/SoC/PSRAM/Source/psram.c
    ${ROOT}/Drivers/SoC/I2CSoft/Source/i2c_soft.c
    ${ROOT}/Drivers/External/OV5640/Source/ov5640.c
    ${PROJECT_DIR}/Src/main.c
    ${PROJECT_DIR}/Src/ui_display.c
    ${PROJECT_DIR}/Src/app_mailbox.c
    ${PROJECT_DIR}/Src/camera_ov5640.c
)

set_source_files_properties(${DEVICE_DIR}/Source/GCC/startup_S300.S PROPERTIES LANGUAGE ASM)

add_executable(${TARGET} ${SRCS})

# Linker script
set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-T${LD_SCRIPT}")

# Link libraries (including drivers and LVGL)
target_link_libraries(${TARGET} PRIVATE
    lvgl
    s300_video
    s300_mailbox
)

# Post build artifacts (bin/hex/disasm/size)
add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
    COMMAND arm-none-eabi-objdump -d -S -C -M force-thumb,reg-names-std -j .text $<TARGET_FILE:${TARGET}> > ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.dis
    COMMAND arm-none-eabi-size $<TARGET_FILE:${TARGET}>
    COMMENT "Generating binary, hex, disassembly and size info"
)

# Debug target
set(GDB_PORT 3333 CACHE STRING "GDB server port")
add_custom_target(dbg_lvgl_demo
    COMMENT "Launching GDB for ${TARGET} (SRAM: 0x20000000) (GDB_PORT=${GDB_PORT})"
    COMMAND arm-none-eabi-gdb -q
    -ex "file $<TARGET_FILE:${TARGET}>"
    -ex "target extended-remote :${GDB_PORT}"
    -x ${ROOT}/gdbinit.display.gdb
    DEPENDS ${TARGET}
    USES_TERMINAL
)

# Debug target with DSP boot images preloaded
# Use modular GDB scripts to allow switching models
macro(add_dsp_debug_target model_name display_name folder_name)
    add_custom_target(dbg_lvgl_demo_${model_name}
        COMMENT "Launching GDB for ${TARGET} with ${display_name} DSP images"
        COMMAND arm-none-eabi-gdb -q
        -ex "file $<TARGET_FILE:${TARGET}>"
        -ex "target extended-remote :${GDB_PORT}"
        -x ${ROOT}/gdbinit.dsp_pre.gdb
        -x ./load.gdb
        -x ${ROOT}/gdbinit.dsp_post.gdb
        WORKING_DIRECTORY ${ROOT}/DSP_Images/${folder_name}
        DEPENDS ${TARGET}
        USES_TERMINAL
    )
endmacro()

add_dsp_debug_target(face_detect "Face Detection" "Face_Detection")
add_dsp_debug_target(face_recog "Face Recognition" "Face_Recognition")
add_dsp_debug_target(hand_gesture "Hand Gesture" "Hand_Gesture")
add_dsp_debug_target(human_detect "Human Detection" "Human_Detection")

# Legacy target alias (defaults to face detection)
add_custom_target(dbg_lvgl_demo_dsp
    COMMENT "Launching GDB for ${TARGET} with default DSP images (Face Detection)"
    COMMAND ${CMAKE_COMMAND} --build . --target dbg_lvgl_demo_face_detect
    DEPENDS dbg_lvgl_demo_face_detect
)
