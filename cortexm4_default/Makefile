# 工程名称和目录定义
PROJECT_NAME := ne004-plus
BUILD_DIR := build
LIBRARIES_DIR := Libraries
CMSIS_DIR := $(LIBRARIES_DIR)/CMSIS
NE004_DRIVER_DIR := $(LIBRARIES_DIR)/NE004xx_Driver

# 工具链路径配置（可以在命令行中覆盖）
TOOLCHAIN_PATH ?= /home/xinhao/work/tflite-micro/tensorflow/lite/micro/tools/make/downloads/gcc_embedded/bin

# 编译工具链设置
CC := $(TOOLCHAIN_PATH)/arm-none-eabi-gcc
CXX := $(TOOLCHAIN_PATH)/arm-none-eabi-g++
AS := $(TOOLCHAIN_PATH)/arm-none-eabi-as
LD := $(TOOLCHAIN_PATH)/arm-none-eabi-ld
OBJCOPY := $(TOOLCHAIN_PATH)/arm-none-eabi-objcopy
OBJDUMP := $(TOOLCHAIN_PATH)/arm-none-eabi-objdump
SIZE := $(TOOLCHAIN_PATH)/arm-none-eabi-size

# 目标架构设置
CPU := -mcpu=cortex-m4
FPU := -mfpu=fpv4-sp-d16
FLOAT-ABI := -mfloat-abi=softfp
MCU := $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# 编译选项
CFLAGS := $(MCU) -g -Wall -Wextra -O2 -fdata-sections -ffunction-sections -MMD -MP
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := $(MCU) -g -x assembler-with-cpp
LDFLAGS := $(MCU) -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(PROJECT_NAME).map -T$(CMSIS_DIR)/NE004/NE004PLUS/Source/GCC/link.ld

# 包含路径
INCLUDES := \
	-I. \
	-I$(CMSIS_DIR)/NE004/NE004PLUS/Include \
	-I$(CMSIS_DIR) \
	-I$(NE004_DRIVER_DIR)/Include

# 源文件和对象文件
C_SOURCES := \
	hal_init.c \
	$(NE004_DRIVER_DIR)/Source/ne004xx_uart.c \
	$(CMSIS_DIR)/NE004/NE004PLUS/Source/system_ne004xx.c \
	syscall.c

CXX_SOURCES := main.cpp

ASM_SOURCES := \
	$(CMSIS_DIR)/NE004/NE004PLUS/Source/GCC/startup_ne004plus.S

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
		   $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES)) \
		   $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

# 默认目标
all: $(BUILD_DIR) $(BUILD_DIR)/$(PROJECT_NAME).elf

# 创建 build 目录的规则
$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(PROJECT_NAME).bin
	$(SIZE) $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

# 包含依赖文件
-include $(wildcard $(BUILD_DIR)/*.d)
