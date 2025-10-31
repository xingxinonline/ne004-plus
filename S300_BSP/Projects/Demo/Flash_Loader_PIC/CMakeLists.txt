# Flash Loader PIC Demo - CMake Configuration
# Fully integrated with S300 BSP build system using CMake + Ninja

cmake_minimum_required(VERSION 3.15)

# Use parent project's compiler settings
# S300 BSP paths (already set by parent CMakeLists.txt)
if(NOT DEFINED BSP_ROOT)
    set(BSP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../..)
endif()

set(CMSIS_DIR ${BSP_ROOT}/CMSIS)
set(DRIVERS_DIR ${BSP_ROOT}/Drivers)
set(BOARDS_DIR ${BSP_ROOT}/Boards/generic_evb)

# ##############################################################################
# Step 1: Build PIC assembly code using CMake custom commands
# ##############################################################################
set(FLASH_OPS_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/flash_ops.S)
set(FLASH_OPS_LINKER ${CMAKE_CURRENT_SOURCE_DIR}/flash_ops.ld)
set(FLASH_OPS_OBJ ${CMAKE_CURRENT_BINARY_DIR}/flash_ops.o)
set(FLASH_OPS_ELF ${CMAKE_CURRENT_BINARY_DIR}/flash_ops.elf)
set(FLASH_OPS_BIN ${CMAKE_CURRENT_BINARY_DIR}/flash_ops.bin)
set(FLASH_OPS_INC ${CMAKE_CURRENT_BINARY_DIR}/flash_ops.inc)
set(FLASH_OPS_LST ${CMAKE_CURRENT_BINARY_DIR}/flash_ops.lst)
set(FLASH_OPS_DIS ${CMAKE_CURRENT_BINARY_DIR}/flash_ops.dis)

# Assemble flash_ops.S with PIC flags
add_custom_command(
    OUTPUT ${FLASH_OPS_OBJ} ${FLASH_OPS_LST}
    COMMAND ${CMAKE_C_COMPILER}
    -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
    -fPIC -fno-common
    -Wa,-adhlmn=${FLASH_OPS_LST}
    -o ${FLASH_OPS_OBJ}
    -c ${FLASH_OPS_SOURCE}
    DEPENDS ${FLASH_OPS_SOURCE}
    COMMENT "Assembling flash_ops.S with PIC flags"
    VERBATIM
)

# Link to ELF at address 0 for position independence
add_custom_command(
    OUTPUT ${FLASH_OPS_ELF}
    COMMAND ${CMAKE_LINKER}
    -s
    --defsym=_start=0
    -T ${FLASH_OPS_LINKER}
    -o ${FLASH_OPS_ELF}
    ${FLASH_OPS_OBJ}
    DEPENDS ${FLASH_OPS_OBJ} ${FLASH_OPS_LINKER}
    COMMENT "Linking flash_ops.elf at address 0"
    VERBATIM
)

# Generate disassembly
add_custom_command(
    OUTPUT ${FLASH_OPS_DIS}
    COMMAND ${CMAKE_OBJDUMP}
    -d ${FLASH_OPS_ELF}
    > ${FLASH_OPS_DIS}
    DEPENDS ${FLASH_OPS_ELF}
    COMMENT "Generating disassembly flash_ops.dis"
    VERBATIM
)

# Extract binary
add_custom_command(
    OUTPUT ${FLASH_OPS_BIN}
    COMMAND ${CMAKE_OBJCOPY}
    -O binary
    ${FLASH_OPS_ELF}
    ${FLASH_OPS_BIN}
    DEPENDS ${FLASH_OPS_ELF}
    COMMENT "Extracting flash_ops.bin"
    VERBATIM
)

# Convert binary to C include file using CMake script
add_custom_command(
    OUTPUT ${FLASH_OPS_INC}
    COMMAND ${CMAKE_COMMAND}
    -DBIN_FILE=${FLASH_OPS_BIN}
    -DINC_FILE=${FLASH_OPS_INC}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/bin2inc.cmake
    DEPENDS ${FLASH_OPS_BIN}
    COMMENT "Converting flash_ops.bin to C include file"
    VERBATIM
)

# Custom target to build all PIC files
add_custom_target(flash_ops_pic ALL
    DEPENDS ${FLASH_OPS_INC} ${FLASH_OPS_DIS} ${FLASH_OPS_LST}
)

# ##############################################################################
# Step 2: Build main executable
# ##############################################################################

# Main program sources
set(MAIN_SOURCES
    main.c
    ${BSP_ROOT}/CMSIS/Device/PiMCHIP/S300/Source/GCC/startup_S300.S
    ${BSP_ROOT}/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c
)

# Mark assembly file
set_source_files_properties(
    ${BSP_ROOT}/CMSIS/Device/PiMCHIP/S300/Source/GCC/startup_S300.S
    PROPERTIES LANGUAGE ASM
)

# Create executable
add_executable(flash_loader_pic.elf ${MAIN_SOURCES})

# Include generated directory for flash_ops.inc
target_include_directories(flash_loader_pic.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
    ${CMAKE_CURRENT_BINARY_DIR}
    ${BSP_ROOT}
    ${CMSIS_DIR}/Core/Include
    ${CMSIS_DIR}/Device/PiMCHIP/S300/Include
    ${DRIVERS_DIR}/SoC/GPIO/Include
    ${DRIVERS_DIR}/SoC/UART/Include
    ${DRIVERS_DIR}/SoC/QSPI/Include
    ${DRIVERS_DIR}/SoC/RCC/Include
    ${BOARDS_DIR}
)

# Link with BSP libraries (use existing library targets if available)
target_sources(flash_loader_pic.elf PRIVATE
    ${BOARDS_DIR}/board.c
    ${BOARDS_DIR}/retarget.c
    ${BOARDS_DIR}/syscalls.c
    ${DRIVERS_DIR}/SoC/GPIO/Source/gpio.c
    ${DRIVERS_DIR}/SoC/UART/Source/uart.c
    ${DRIVERS_DIR}/SoC/QSPI/Source/qspi_cadence.c
    ${DRIVERS_DIR}/SoC/RCC/Source/rcc.c
)

# Linker script
set(LINKER_SCRIPT ${BSP_ROOT}/ld/sram.ld)
target_link_options(flash_loader_pic.elf PRIVATE
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/flash_loader_pic.map
)

# Make main executable depend on PIC code generation
add_dependencies(flash_loader_pic.elf flash_ops_pic)

# Post-build: create binary
add_custom_command(TARGET flash_loader_pic.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY}
    -O binary
    flash_loader_pic.elf
    flash_loader_pic.bin
    COMMENT "Creating flash_loader_pic.bin"
)

# ##############################################################################
# Step 3: Debug target
# ##############################################################################

# Debug target: Launch GDB with OpenOCD for SRAM debugging
set(GDB_PORT 3333 CACHE STRING "GDB server port")
add_custom_target(dbg_flash_loader_pic
    COMMENT "Launching GDB for flash_loader_pic.elf (SRAM: 0x20000000) (GDB_PORT=${GDB_PORT})"
    COMMAND arm-none-eabi-gdb -q
    -ex "file $<TARGET_FILE:flash_loader_pic.elf>"
    -ex "target extended-remote :${GDB_PORT}"
    -x ${BSP_ROOT}/gdbinit.sram.gdb
    DEPENDS flash_loader_pic.elf
    USES_TERMINAL
)

# Print build info
message(STATUS "Flash Loader PIC Configuration:")
message(STATUS "  PIC Source:    ${FLASH_OPS_SOURCE}")
message(STATUS "  PIC Binary:    ${FLASH_OPS_BIN}")
message(STATUS "  PIC Include:   ${FLASH_OPS_INC}")
message(STATUS "  Linker Script: ${LINKER_SCRIPT}")
