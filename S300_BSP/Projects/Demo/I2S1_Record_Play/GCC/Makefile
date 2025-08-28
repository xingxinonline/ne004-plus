TARGET      := s300_demo_i2s1_record_play
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
ROOT        := $(abspath $(CURDIR)/../../../..)
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
LD_SCRIPT   := $(ROOT)/ld/sram.ld

CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

.RECIPEPREFIX := >

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-O2 -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra \
	-I$(DEVICE_DIR)/Include -I$(ROOT)/CMSIS/Core/Include \
	-I$(ROOT)/Boards/generic_evb \
	-I$(ROOT)/Drivers/SoC/RCC/Include -I$(ROOT)/Drivers/SoC/GPIO/Include \
	-I$(ROOT)/Drivers/SoC/UART/Include \
	-I$(ROOT)/Drivers/SoC/DMA/Include -I$(ROOT)/Drivers/SoC/I2S/Include \
	-I$(ROOT)/Drivers/SoC/I2CSoft/Include -I$(ROOT)/Drivers/External/WM8978/Include
ASFLAGS := $(CFLAGS)

LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	-Wl,-u,_printf_float -Wl,-u,_scanf_float -Wl,-u,_printf_long_long -Wl,-u,_scanf_long_long

SRCS := \
	$(DEVICE_DIR)/Source/GCC/startup_S300.S \
	$(DEVICE_DIR)/Source/system_S300.c \
	$(PROJECT_DIR)/Src/main.c \
			$(ROOT)/Boards/generic_evb/board.c \
			$(ROOT)/Boards/generic_evb/retarget.c \
			$(ROOT)/Boards/generic_evb/syscalls.c \
	$(ROOT)/Drivers/SoC/RCC/Source/rcc.c \
	$(ROOT)/Drivers/SoC/GPIO/Source/gpio.c \
	$(ROOT)/Drivers/SoC/UART/Source/uart.c \
	$(ROOT)/Drivers/SoC/DMA/Source/dma.c \
	$(ROOT)/Drivers/SoC/I2S/Source/i2s.c \
	$(ROOT)/Drivers/SoC/I2CSoft/Source/i2c_soft.c \
	$(ROOT)/Drivers/External/WM8978/Source/wm8978.c

SRCS_REL := $(patsubst $(ROOT)/%,%,$(SRCS))
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS_REL))
OBJS := $(patsubst %.c,%.o,$(OBJS))
OBJS := $(patsubst %.S,%.o,$(OBJS))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: $(ROOT)/%.c | prep
> @mkdir -p $(dir $@)
> $(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(ROOT)/%.S | prep
> @mkdir -p $(dir $@)
> $(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
> $(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
> $(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
> $(OBJCOPY) -O binary $< $@

prep:
> @mkdir -p $(BUILD_DIR)

clean:
> rm -rf $(BUILD_DIR)

.PHONY: all clean prep dbg

dbg: all
> $(MAKE) -C $(ROOT) dbg PROJECT=$(notdir $(PROJECT_DIR))
