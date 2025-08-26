TARGET      := s300_minimal
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
ROOT        := $(PROJECT_DIR)/../..
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
LD_SCRIPT   := $(ROOT)/ld/sram.ld

CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
		   -O2 -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra \
		   -I$(DEVICE_DIR)/Include -I$(ROOT)/CMSIS/Core/Include
ASFLAGS := $(CFLAGS)
LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

SRCS := \
  $(DEVICE_DIR)/Source/GCC/startup_S300.S \
  $(DEVICE_DIR)/Source/system_S300.c \
	$(PROJECT_DIR)/Src/main.c \
	$(PROJECT_DIR)/Src/retarget.c
LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map
# Link with newlib nano and enable float printf support
LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
		   -Wl,-u,_printf_float -Wl,-u,_scanf_float -Wl,-u,_printf_long_long -Wl,-u,_scanf_long_long

OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | prep
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.S | prep
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

prep:
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean prep dbg

dbg: all
	$(MAKE) -C $(ROOT) dbg PROJECT=$(notdir $(PROJECT_DIR))
