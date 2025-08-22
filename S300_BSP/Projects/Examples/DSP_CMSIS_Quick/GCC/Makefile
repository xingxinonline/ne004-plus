# Example: CMSIS-DSP Quick Check on S300 (Cortex-M4)

PROJECT := dsp_cmsis_quick
BUILD   := build
ROOT    := ../../../../
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy

# Roots
CMSIS_CORE_ROOT := $(ROOT)/CMSIS/Core/Include
CMSIS_DSP_ROOT  := $(abspath $(CURDIR)/../../../../../CMSIS-DSP)

# Common compile flags
CFLAGS  := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Ofast -ffast-math -ffunction-sections -fdata-sections \
	-I$(ROOT)/CMSIS/Device/PiMCHIP/S300/Include -I$(CMSIS_CORE_ROOT) -I$(ROOT)/Drivers/BSP/Include \
	-I$(ROOT)/Drivers/UART/Include -I$(ROOT)/Boards/S300_EVB -I$(ROOT)/Drivers/SoC/Include

# 加载通用 CMSIS-DSP 集成脚本（可通过 CMSIS_DSP_ROOT 覆盖路径）
include $(ROOT)/tools/cmsis_dsp.mk

# 追加 CMSIS-DSP CFLAGS
CFLAGS += $(CMSIS_DSP_CFLAGS)

ASFLAGS := $(CFLAGS)
LDFLAGS := -T $(ROOT)/ld/sram.ld -Wl,--gc-sections -nostartfiles -Wl,-Map,$(BUILD)/$(PROJECT).map

# Common BSP settings (retarget + printf/scanf float & long long)
include $(ROOT)/tools/bsp_common.mk

# Sources
SRCS := \
	../Src/main.c \
	$(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c \
	$(ROOT)/Drivers/UART/Source/s300_uart.c \
	$(ROOT)/Boards/S300_EVB/clock_config.c \
	$(ROOT)/Boards/S300_EVB/pinmux_config.c \
	$(BSP_COMMON_SRCS)

SRCS_S := \
	$(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/GCC/startup_S300.S

# Objects
BSP_OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS:.c=.o)))
ASM_OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS_S:.S=.o)))
OBJS := $(BSP_OBJS) $(ASM_OBJS) $(CMSIS_DSP_OBJS)

# Rules
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

# CMSIS-DSP 构建规则由 tools/cmsis_dsp.mk 提供

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
