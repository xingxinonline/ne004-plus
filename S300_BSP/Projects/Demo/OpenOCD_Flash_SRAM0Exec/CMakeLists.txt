cmake_minimum_required(VERSION 3.16)

project(OpenOCD_Flash_SRAM0Exec)

set(TARGET s300_openocd_flash_sram0exec)

set(PROJECT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ROOT ${PROJECT_DIR}/../../..)
set(DEVICE_DIR ${ROOT}/CMSIS/Device/PiMCHIP/S300)
set(LD_SCRIPT ${PROJECT_DIR}/ld/sram0_exec.ld)

include_directories(
    ${DEVICE_DIR}/Include
    ${ROOT}/CMSIS/Core/Include
    ${ROOT}/Drivers/SoC/RCC/Include
    ${ROOT}/Drivers/SoC/GPIO/Include
    ${ROOT}/Drivers/SoC/UART/Include
    ${ROOT}/Drivers/SoC/QSPI/Include
    ${ROOT}/Drivers/External/W25Qxx/Include
    ${ROOT}/Boards/generic_evb
    ${PROJECT_DIR} # FreeRTOSConfig.h
)

set(SRCS
    ${DEVICE_DIR}/Source/GCC/startup_S300.S
    ${ROOT}/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c
    ${ROOT}/Boards/generic_evb/board.c
    ${ROOT}/Boards/generic_evb/retarget.c
    ${ROOT}/Boards/generic_evb/syscalls.c
    ${ROOT}/Drivers/SoC/RCC/Source/rcc.c
    ${ROOT}/Drivers/SoC/GPIO/Source/gpio.c
    ${ROOT}/Drivers/SoC/UART/Source/uart.c
    ${ROOT}/Drivers/SoC/QSPI/Source/qspi_cadence.c
    ${ROOT}/Drivers/External/W25Qxx/Source/w25qxx.c
    ${PROJECT_DIR}/Src/loader.c
    ${PROJECT_DIR}/Src/main.c
)

set_source_files_properties(${DEVICE_DIR}/Source/GCC/startup_S300.S PROPERTIES LANGUAGE ASM)

add_executable(${TARGET} ${SRCS})

# FreeRTOS integration
set(FREERTOS_KERNEL_LOCAL_PATH "" CACHE PATH "Local FreeRTOS-Kernel override")
set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "FreeRTOS port")
set(FREERTOS_HEAP 4 CACHE STRING "FreeRTOS heap implementation")
set(FREERTOS_CONFIG_FILE_DIRECTORY ${PROJECT_DIR} CACHE PATH "Path to FreeRTOSConfig.h")

if(FREERTOS_KERNEL_LOCAL_PATH AND EXISTS ${FREERTOS_KERNEL_LOCAL_PATH}/CMakeLists.txt)
    message(STATUS "Using FreeRTOS-Kernel from local path: ${FREERTOS_KERNEL_LOCAL_PATH}")
    set(FREERTOS_KERNEL_DIR ${FREERTOS_KERNEL_LOCAL_PATH})
else()
    include(FetchContent)
    FetchContent_Declare(
        freertos_kernel
        GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
        GIT_TAG V10.5.1
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(freertos_kernel)
    set(FREERTOS_KERNEL_DIR ${freertos_kernel_SOURCE_DIR})
endif()

set(FREERTOS_PORT_DIR ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F)

set(FREERTOS_SRCS
    ${FREERTOS_KERNEL_DIR}/tasks.c
    ${FREERTOS_KERNEL_DIR}/queue.c
    ${FREERTOS_KERNEL_DIR}/list.c
    ${FREERTOS_KERNEL_DIR}/timers.c
    ${FREERTOS_KERNEL_DIR}/event_groups.c
    ${FREERTOS_KERNEL_DIR}/stream_buffer.c
    ${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_4.c
    ${FREERTOS_PORT_DIR}/port.c
)

target_sources(${TARGET} PRIVATE ${FREERTOS_SRCS})

target_include_directories(${TARGET} PRIVATE
    ${FREERTOS_KERNEL_DIR}/include
    ${FREERTOS_PORT_DIR}
)

set_target_properties(${TARGET} PROPERTIES LINK_FLAGS "-T${LD_SCRIPT}")

add_custom_command(TARGET ${TARGET} POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.bin
    COMMAND arm-none-eabi-objcopy -O ihex $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.hex
    COMMAND arm-none-eabi-objdump -d -S -C -M force-thumb,reg-names-std -j .text -j .loader_text $<TARGET_FILE:${TARGET}> > ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.dis
    COMMAND arm-none-eabi-size $<TARGET_FILE:${TARGET}>
    COMMENT "Generating binary, hex, disassembly and size info for ${TARGET}"
)

set(GDB_PORT 3333 CACHE STRING "GDB server port")
add_custom_target(dbg_openocd_flash_sram0exec
    COMMENT "Launching GDB for ${TARGET} (SRAM loader demo)"
    COMMAND arm-none-eabi-gdb -q
    -ex "file $<TARGET_FILE:${TARGET}>"
    -ex "target extended-remote :${GDB_PORT}"
    -x ${ROOT}/gdbinit.sram.gdb
    DEPENDS ${TARGET}
    USES_TERMINAL
)
