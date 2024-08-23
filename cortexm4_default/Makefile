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

ADDITIONAL_DEFINES :=

CC_WARNINGS := \
	-Wsign-compare \
	-Wdouble-promotion \
	-Wunused-variable \
	-Wunused-function \
	-Wswitch \
	-Wvla \
	-Wall \
	-Wextra \
	-Wmissing-field-initializers \
	-Wstrict-aliasing \
	-Wno-unused-parameter

COMMON_FLAGS := \
	-Werror \
	-fno-unwind-tables \
	-ffunction-sections \
	-fdata-sections \
	-fmessage-length=0 \
	-DTF_LITE_STATIC_MEMORY \
	-DTF_LITE_DISABLE_X86_NEON \
	$(CC_WARNINGS) \
	$(ADDITIONAL_DEFINES)

CXXFLAGS := \
	-std=c++17 \
	-fno-rtti \
	-fno-exceptions \
	-fno-threadsafe-statics \
	-Wnon-virtual-dtor \
	$(COMMON_FLAGS)

CCFLAGS := \
	-Wimplicit-function-declaration \
	-std=c17 \
	$(COMMON_FLAGS)

ARFLAGS := -r

LDFLAGS += \
	-Wl,--fatal-warnings \
	-Wl,--gc-sections

CXXFLAGS += -g
CCFLAGS  += -g

CORE=M4
ARM_LDFLAGS := -Wl,--cpu=Cortex-M4
CMSIS_ARM_FEATURES := _FP
FLOAT=softfp
GCC_TARGET_ARCH := cortex-m4

ARM_CPU := "ARMC$(CORE)"

FLAGS_GCC = -mcpu=$(GCC_TARGET_ARCH)

CXXFLAGS += $(FLAGS_GCC)
CCFLAGS += $(FLAGS_GCC)

PLATFORM_FLAGS = \
	-DTF_LITE_MCU_DEBUG_LOG \
	-mthumb \
	-mfloat-abi=$(FLOAT) \
	-funsigned-char \
	-mlittle-endian \
	-Wno-type-limits \
	-Wno-unused-private-field \
	-fomit-frame-pointer \
	-MD \
	-DCPU_$(CORE)=1

# Common + C/C++ flags
CXXFLAGS += $(PLATFORM_FLAGS)
CCFLAGS += $(PLATFORM_FLAGS)
# LDFLAGS += $(ARM_LDFLAGS)

# 编译选项
CCFLAGS += -O0
CXXFLAGS += -O0 
LDFLAGS += $(MCU) -Wl,--fatal-warnings -Wl,-Map=$(BUILD_DIR)/$(PROJECT_NAME).map -T$(CMSIS_DIR)/NE004/NE004PLUS/Source/GCC/link.ld

# 包含库路径和库名
LIB_DIR := lib
LIBS := -L$(LIB_DIR) -ltensorflow-microlite

# 包含路径
INCLUDES := \
	-I. \
	-I$(CMSIS_DIR)/NE004/NE004PLUS/Include \
	-I$(CMSIS_DIR) \
	-I$(NE004_DRIVER_DIR)/Include \
	-Iinclude

# 源文件和对象文件
C_SOURCES := \
	hal_init.c \
	$(NE004_DRIVER_DIR)/Source/ne004xx_uart.c \
	$(CMSIS_DIR)/NE004/NE004PLUS/Source/system_ne004xx.c \
	syscall.c 

CXX_SOURCES := \
			hello_world_int8_model_data.cpp \
			main.cpp 

ASM_SOURCES := \
	$(CMSIS_DIR)/NE004/NE004PLUS/Source/GCC/startup_ne004plus.S

OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES)) \
		   $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CXX_SOURCES)) \
		   $(patsubst %.S,$(BUILD_DIR)/%.o,$(ASM_SOURCES))

# 默认目标
all: $(BUILD_DIR) $(BUILD_DIR)/$(PROJECT_NAME).elf $(BUILD_DIR)/$(PROJECT_NAME).dis $(BUILD_DIR)/$(PROJECT_NAME).all.dis

# 创建 build 目录的规则
$(BUILD_DIR):
	@mkdir -p $@

$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LIBS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD_DIR)/$(PROJECT_NAME).bin
	$(SIZE) $@

# 使用 objdump 生成 .dis 文件
$(BUILD_DIR)/$(PROJECT_NAME).dis: $(BUILD_DIR)/$(PROJECT_NAME).elf
	$(OBJDUMP) -d $< > $@

$(BUILD_DIR)/$(PROJECT_NAME).all.dis: $(BUILD_DIR)/$(PROJECT_NAME).elf
	$(OBJDUMP) -D -S $< > $@

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)/*

# 包含依赖文件
-include $(wildcard $(BUILD_DIR)/*.d)
