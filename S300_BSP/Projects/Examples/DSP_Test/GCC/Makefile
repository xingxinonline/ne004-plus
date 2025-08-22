# Example: DSP Instruction Test on S300 (Cortex-M4)

PROJECT := dsp_test
BUILD   := build
ROOT    := ../../../../
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy

# Roots
# CMSIS Core is provided inside S300_BSP/CMSIS
CMSIS_CORE_ROOT := $(ROOT)/CMSIS/Core/Include
# CMSIS-DSP repository is at workspace root
CMSIS_DSP_ROOT  := $(abspath $(CURDIR)/../../../../../CMSIS-DSP)

# Common compile flags
CFLAGS  := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Ofast -ffast-math -ffunction-sections -fdata-sections \
	-I$(ROOT)/CMSIS/Device/PiMCHIP/S300/Include -I$(CMSIS_CORE_ROOT) -I$(ROOT)/Drivers/BSP/Include \
	-I$(ROOT)/Drivers/UART/Include -I$(ROOT)/Boards/S300_EVB -I$(ROOT)/Drivers/SoC/Include \
	-I$(CMSIS_DSP_ROOT)/Include -I$(CMSIS_DSP_ROOT)/PrivateInclude

# CMSIS-DSP specific macros
# ARM_MATH_CM4 selects CM4 optimized codepaths; DISABLEFLOAT16 avoids f16 build on M4
CFLAGS  += -DARM_MATH_CM4 -DDISABLEFLOAT16

ASFLAGS := $(CFLAGS)
LDFLAGS := -T $(ROOT)/ld/sram.ld -Wl,--gc-sections -nostartfiles -Wl,-Map,$(BUILD)/$(PROJECT).map

# Common BSP settings (retarget + printf/scanf float & long long)
include $(ROOT)/tools/bsp_common.mk

BSP_SRCS_C := \
	../Src/main.c \
	$(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c \
	$(ROOT)/Drivers/UART/Source/s300_uart.c \
	$(ROOT)/Boards/S300_EVB/clock_config.c \
	$(ROOT)/Boards/S300_EVB/pinmux_config.c \
	$(BSP_COMMON_SRCS)

# Aggregate CMSIS-DSP sources (each file includes all kernels in its folder)
DSP_SRCS := \
	$(CMSIS_DSP_ROOT)/Source/BasicMathFunctions/BasicMathFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/CommonTables/CommonTables.c \
	$(CMSIS_DSP_ROOT)/Source/InterpolationFunctions/InterpolationFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/BayesFunctions/BayesFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/MatrixFunctions/MatrixFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/ComplexMathFunctions/ComplexMathFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/QuaternionMathFunctions/QuaternionMathFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/ControllerFunctions/ControllerFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/SVMFunctions/SVMFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/DistanceFunctions/DistanceFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/StatisticsFunctions/StatisticsFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/FastMathFunctions/FastMathFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/SupportFunctions/SupportFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/FilteringFunctions/FilteringFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/TransformFunctions/TransformFunctions.c \
	$(CMSIS_DSP_ROOT)/Source/WindowFunctions/WindowFunctions.c

SRCS_S := \
	$(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/GCC/startup_S300.S

# Objects
BSP_OBJS := $(addprefix $(BUILD)/,$(notdir $(BSP_SRCS_C:.c=.o)))
DSP_OBJS := $(addprefix $(BUILD)/,$(patsubst $(CMSIS_DSP_ROOT)/Source/%,%,$(DSP_SRCS:.c=.o)))
ASM_OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS_S:.S=.o)))
OBJS := $(BSP_OBJS) $(DSP_OBJS) $(ASM_OBJS)

$(BUILD)/%.o: ../Src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(ROOT)/Drivers/UART/Source/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(ROOT)/Drivers/BSP/Source/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(ROOT)/Boards/S300_EVB/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/GCC/%.S | $(BUILD)
	$(AS) $(ASFLAGS) -c $< -o $@

# CMSIS-DSP sources (preserve subdir under build/). Use single % to match subpath
$(BUILD)/%.o: $(CMSIS_DSP_ROOT)/Source/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

all: $(BUILD)/$(PROJECT).elf

$(BUILD)/$(PROJECT).elf: $(OBJS) | $(BUILD)
	$(LD) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD)/$(PROJECT).bin

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: all clean dbg

dbg: all
	arm-none-eabi-gdb -q -x gdbinit.gdb
