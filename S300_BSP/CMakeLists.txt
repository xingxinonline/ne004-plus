cmake_minimum_required(VERSION 3.16)

# Set the toolchain for ARM Cortex-M4
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Specify the cross compiler
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_C_COMPILER D:/Scoop/apps/gcc-arm-none-eabi/current/bin/arm-none-eabi-gcc.exe)
    set(CMAKE_CXX_COMPILER D:/Scoop/apps/gcc-arm-none-eabi/current/bin/arm-none-eabi-g++.exe)
    set(CMAKE_ASM_COMPILER D:/Scoop/apps/gcc-arm-none-eabi/current/bin/arm-none-eabi-gcc.exe)
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(CMAKE_C_COMPILER /home/xinhao/crosstools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc)
    set(CMAKE_CXX_COMPILER /home/xinhao/crosstools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-g++)
    set(CMAKE_ASM_COMPILER /home/xinhao/crosstools/gcc-arm-none-eabi-10.3-2021.10/bin/arm-none-eabi-gcc)
else()
    message(FATAL_ERROR "Unsupported host system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

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

# Add debug target (equivalent to make dbg)
# GDB port can be customized; default OpenOCD/J-Link is :3333
set(GDB_PORT 3333 CACHE STRING "GDB server port")

# Find all ELF files in the build directory
file(GLOB_RECURSE ELF_FILES "${CMAKE_BINARY_DIR}/Projects/*" "*")

# Filter for ELF executables (no extension)
set(ELF_TARGETS "")

foreach(elf_file ${ELF_FILES})
    get_filename_component(elf_name ${elf_file} NAME)

    if(NOT elf_name MATCHES "\\.")
        list(APPEND ELF_TARGETS ${elf_file})
    endif()
endforeach()

# Create debug targets for each project
foreach(elf_file ${ELF_TARGETS})
    get_filename_component(elf_name ${elf_file} NAME)
    get_filename_component(elf_dir ${elf_file} DIRECTORY)
    get_filename_component(project_name ${elf_dir} NAME)

    # Create project-specific debug target (e.g., dbg_rbl, dbg_sbl)
    if(project_name STREQUAL "RBL_Minimal")
        add_custom_target(dbg_rbl
            COMMENT "Launching GDB for ${elf_name} (RBL) (GDB_PORT=${GDB_PORT})"
            COMMAND arm-none-eabi-gdb -q -ex "file ${elf_file}" -ex "target extended-remote :${GDB_PORT}" -x ${CMAKE_SOURCE_DIR}/gdbinit.common.gdb
            DEPENDS ${elf_file}
            USES_TERMINAL
        )
    elseif(project_name STREQUAL "SBL_Minimal")
        add_custom_target(dbg_sbl
            COMMENT "Launching GDB for ${elf_name} (SBL) (GDB_PORT=${GDB_PORT})"
            COMMAND arm-none-eabi-gdb -q -ex "file ${elf_file}" -ex "target extended-remote :${GDB_PORT}" -x ${CMAKE_SOURCE_DIR}/gdbinit.common.gdb
            DEPENDS ${elf_file}
            USES_TERMINAL
        )
    endif()
endforeach()

if(ELF_TARGETS)
    # Get the most recently modified ELF file
    list(GET ELF_TARGETS 0 LATEST_ELF)

    foreach(elf_file ${ELF_TARGETS})
        if(${elf_file} IS_NEWER_THAN ${LATEST_ELF})
            set(LATEST_ELF ${elf_file})
        endif()
    endforeach()

    get_filename_component(ELF_NAME ${LATEST_ELF} NAME)

    # Create general debug target that uses the latest ELF
    add_custom_target(dbg
        COMMENT "Launching GDB for ${ELF_NAME} (latest) (GDB_PORT=${GDB_PORT})"
        COMMAND arm-none-eabi-gdb -q -ex "file ${LATEST_ELF}" -ex "target extended-remote :${GDB_PORT}" -x ${CMAKE_SOURCE_DIR}/gdbinit.common.gdb
        DEPENDS ${LATEST_ELF}
        USES_TERMINAL
    )
endif()

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()