# Drivers CMakeLists.txt
# Build SoC driver libraries (minimal subset for video/display)

set(DRIVERS_ROOT ${CMAKE_CURRENT_LIST_DIR})

add_library(s300_video STATIC
    ${DRIVERS_ROOT}/SoC/MM/video.c
)

target_include_directories(s300_video PUBLIC
    ${PROJECT_SOURCE_DIR}/CMSIS/Device/PiMCHIP/S300/Include
    ${PROJECT_SOURCE_DIR}/CMSIS/Core/Include
    ${DRIVERS_ROOT}/SoC/MM
)
