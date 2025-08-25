# S300 DMA RAM to Peripheral(UART1 TX) example

PROJECT := dma_m2p_uart1_tx
BUILD   := build
ROOT    := ../../../../
CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
LD      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
CFLAGS  := -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -O2 -ffunction-sections -fdata-sections
ASFLAGS := $(CFLAGS)
LDFLAGS := -T $(ROOT)/ld/sram.ld -Wl,--gc-sections -nostartfiles -Wl,-Map,$(BUILD)/$(PROJECT).map

include $(ROOT)/tools/bsp_common.mk

SRCS_C := \
	../Src/main.c \
	$(ROOT)/CMSIS/Device/PiMCHIP/S300/Source/system_S300.c \
	$(ROOT)/Drivers/UART/Source/s300_uart.c \
	$(ROOT)/Boards/S300_EVB/clock_config.c \
	$(ROOT)/Boards/S300_EVB/pinmux_config.c \
	$(BSP_COMMON_SRCS)

SRCS_S := \
	../../../../CMSIS/Device/PiMCHIP/S300/Source/GCC/startup_S300.S

OBJS := $(addprefix $(BUILD)/,$(notdir $(SRCS_C:.c=.o))) $(addprefix $(BUILD)/,$(notdir $(SRCS_S:.S=.o)))

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

all: $(BUILD)/$(PROJECT).elf

$(BUILD)/$(PROJECT).elf: $(OBJS) | $(BUILD)
	$(LD) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	$(OBJCOPY) -O binary $@ $(BUILD)/$(PROJECT).bin

$(BUILD):
	@mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)

.PHONY: all clean

dbg: all
	arm-none-eabi-gdb -q -x gdbinit.gdb
