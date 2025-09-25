TARGET      := cortex-m4-reg_rw_test
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
ROOT        := $(abspath $(CURDIR)/../../../..)
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
LD_SCRIPT   := $(ROOT)/ld/sram.ld

CASE_NAME   := reg_rw_test
OUT_DIR     := $(BUILD_DIR)/cortex-m4-$(CASE_NAME)/ram/out

CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
READELF := arm-none-eabi-readelf
SIZE    := arm-none-eabi-size

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-O2 -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra \
	-I$(DEVICE_DIR)/Include -I$(ROOT)/CMSIS/Core/Include \
	-I$(ROOT)/Drivers/SoC/RCC/Include \
	-I$(ROOT)/Drivers/SoC/GPIO/Include
ASFLAGS := $(CFLAGS)

LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
   -Wl,-u,_printf_float -Wl,-u,_scanf_float -Wl,-u,_printf_long_long -Wl,-u,_scanf_long_long

SRCS := \
	$(DEVICE_DIR)/Source/GCC/startup_S300.S \
	$(DEVICE_DIR)/Source/system_S300.c \
	$(PROJECT_DIR)/Src/main.c \
	$(ROOT)/Drivers/SoC/GPIO/Source/gpio.c

SRCS_REL := $(patsubst $(ROOT)/%,%,$(SRCS))
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS_REL))
OBJS := $(patsubst %.c,%.o,$(OBJS))
OBJS := $(patsubst %.S,%.o,$(OBJS))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).dis $(BUILD_DIR)/$(TARGET).all.dis $(BUILD_DIR)/$(TARGET).entrypoint $(BUILD_DIR)/$(TARGET).dua $(BUILD_DIR)/$(TARGET).vmf $(BUILD_DIR)/$(TARGET).rcf $(BUILD_DIR)/$(TARGET).krv copy_to_out

$(BUILD_DIR)/%.o: $(ROOT)/%.c | prep
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(ROOT)/%.S | prep
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).dis: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -D $< > $@

$(BUILD_DIR)/$(TARGET).all.dis: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -D -S $< > $@

$(BUILD_DIR)/$(TARGET).entrypoint: $(BUILD_DIR)/$(TARGET).elf
	$(READELF) -s $< | grep "g_pfnVectors" | sed -n 's/.* \([0-9a-fA-F]\{8\}\) .*g_pfnVectors/0x\1/p' > $@

$(BUILD_DIR)/$(TARGET).dua: $(BUILD_DIR)/$(TARGET).bin
	od -t x4 -An -w4 -v $< > $@

$(BUILD_DIR)/$(TARGET).vmf: $(BUILD_DIR)/$(TARGET).bin
	echo "@00000000" > $@
	od -t x1 -An -w16 -v $< >> $@

$(BUILD_DIR)/$(TARGET).rcf: $(BUILD_DIR)/$(TARGET).bin
	# Placeholder for RCF generation - requires genrom.py script
	touch $@

$(BUILD_DIR)/$(TARGET).krv: $(BUILD_DIR)/$(TARGET).bin
	# Placeholder for KRV generation - requires bin2rom.py script
	touch $@

prep:
	@mkdir -p $(BUILD_DIR)

copy_to_out: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).dis $(BUILD_DIR)/$(TARGET).all.dis $(BUILD_DIR)/$(TARGET).entrypoint $(BUILD_DIR)/$(TARGET).dua $(BUILD_DIR)/$(TARGET).vmf $(BUILD_DIR)/$(TARGET).rcf $(BUILD_DIR)/$(TARGET).krv
	@mkdir -p $(OUT_DIR)
	@cp $(BUILD_DIR)/$(TARGET).elf $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).bin $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).hex $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).dis $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).all.dis $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).entrypoint $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).dua $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).vmf $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).rcf $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).krv $(OUT_DIR)/
	@cp $(BUILD_DIR)/$(TARGET).map $(OUT_DIR)/
	@echo "Files copied to $(OUT_DIR)"
	@cd $(BUILD_DIR) && zip -r $(TARGET).zip cortex-m4-$(CASE_NAME)/
	@echo "Created $(BUILD_DIR)/$(TARGET).zip"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean prep copy_to_out dbg

dbg: all
	# 传递相对于 Projects/ 的路径，如 Demo/QSPI_W25Q128_Test
	$(MAKE) -C $(ROOT) dbg PROJECT=$(patsubst $(ROOT)/Projects/%,%,$(abspath $(PROJECT_DIR)))
