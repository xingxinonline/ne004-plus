# Example: FPU Test on S300

PROJECT := fpu_test
BUILD   := build
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
CFLAGS  := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -O2 -ffunction-sections -fdata-sections \
	-I../../../../CMSIS/Device/PiMCHIP/S300/Include -I../../../../CMSIS/Core/Include -I../../../../Drivers/BSP/Include \
	-I../../../../Drivers/UART/Include -I../../../../Boards/S300_EVB -I../../../../Drivers/SoC/Include
ASFLAGS := $(CFLAGS)
LDFLAGS := -T ../../../../ld/sram.ld -Wl,--gc-sections -nostartfiles -Wl,-Map,$(BUILD)/$(PROJECT).map

SRCS_C := \
	../Src/main.c \
	../../../../CMSIS/Device/PiMCHIP/S300/Source/system_S300.c \
	../../../../Drivers/UART/Source/s300_uart.c \
	../../../../Boards/S300_EVB/clock_config.c \
	../../../../Boards/S300_EVB/pinmux_config.c

SRCS_S := \
	../../../../CMSIS/Device/PiMCHIP/S300/Source/GCC/startup_S300.S

OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS_C:.c=.o))) $(addprefix $(BUILD)/,$(notdir $(SRCS_S:.S=.o)))

$(BUILD)/%.o: ../Src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: ../../../../CMSIS/Device/PiMCHIP/S300/Source/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: ../../../../Drivers/UART/Source/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: ../../../../Boards/S300_EVB/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: ../../../../CMSIS/Device/PiMCHIP/S300/Source/GCC/%.S | $(BUILD)
	$(AS) $(ASFLAGS) -c $< -o $@

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
