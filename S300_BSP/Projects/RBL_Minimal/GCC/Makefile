TARGET      := s300_rbl_minimal
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
ROOT        := $(PROJECT_DIR)/../..
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
# Use local 64KB-limited SRAM linker script for RBL
LD_SCRIPT   := $(PROJECT_DIR)/ld/sram64.ld

# 目标部署目录（可通过 make DEST_DIR=... 覆盖）
DEST_DIR ?= /mnt/g/workspace/next/bin

CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	   -Os -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra \
	   -g3 -gdwarf-2 \
	   -I$(DEVICE_DIR)/Include -I$(ROOT)/CMSIS/Core/Include \
	   -I$(PROJECT_DIR)/Inc \
	   -I$(ROOT)/Drivers/SoC/QSPI/Include \
	   -I$(ROOT)/Drivers/SoC/RCC/Include

# 允许通过 make LOG=0 关闭日志
LOG ?= 1
CFLAGS += -DRBL_LOG_ENABLE=$(LOG)
# 禁用QSPI驱动中的printf调试输出，避免链接复杂的stdio库
CFLAGS += -Dprintf=rbl_printf_stub

ASFLAGS := $(CFLAGS)

# 最小链接选项，使用nosys
LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	   -specs=nosys.specs

# 极简工程：移除下载功能，仅保留启动 + Phase 2/3 modules
SRCS := \
  $(DEVICE_DIR)/Source/GCC/startup_S300.S \
	$(PROJECT_DIR)/Src/rbl_hal.c \
	$(ROOT)/Drivers/SoC/RCC/Source/rcc.c \
	$(ROOT)/Drivers/SoC/QSPI/Source/qspi_cadence.c \
	$(PROJECT_DIR)/Src/rbl_qspi.c \
	$(PROJECT_DIR)/Src/rbl_sbl.c \
	$(PROJECT_DIR)/Src/rbl_fault.c \
	$(PROJECT_DIR)/Src/rbl_simple.c

OBJS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS)))

all: $(BUILD_DIR)/$(TARGET).elf \
	$(BUILD_DIR)/$(TARGET).bin \
	$(BUILD_DIR)/$(TARGET).hex \
	$(BUILD_DIR)/$(TARGET).dis

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

# Intel HEX 输出
$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# 反汇编输出（.dis）：强制Thumb，包含源码注释，仅反汇编.text
$(BUILD_DIR)/$(TARGET).dis: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -d -S -C -M force-thumb,reg-names-std -j .text $< > $@

# S300 header generation (简化命名)
s300_image: $(BUILD_DIR)/$(TARGET).bin
	@echo "Generating S300 Header + Complete image..."
	python3 generate_s300_standard_header.py generate $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET)_header.bin
	python3 merge_s300_image.py $(BUILD_DIR)/$(TARGET)_header.bin $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET)_complete.bin || echo "Merge script not found"
	@echo "S300 complete image: $(BUILD_DIR)/$(TARGET)_complete.bin"
	@mkdir -p $(DEST_DIR)
	@cp -f $(BUILD_DIR)/$(TARGET)_complete.bin $(DEST_DIR)/
	@echo "Copied to $(DEST_DIR)/$(TARGET)_complete.bin"

install: s300_image
	@true

analyze_header: $(BUILD_DIR)/$(TARGET)_header.bin
	@echo "Analyzing S300 header format..."
	python3 analyze_s300_header.py $(BUILD_DIR)/$(TARGET)_header.bin

# GDB 调试：连接到 :3333，并自动load到SRAM
dbg: $(BUILD_DIR)/$(TARGET).elf
	@echo "Starting GDB (expects server at :3333)..."
	arm-none-eabi-gdb -ex "file $(BUILD_DIR)/$(TARGET).elf" -x $(ROOT)/gdbinit.common.gdb

prep:
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean prep s300_image analyze_header help dbg install

help:
	@echo "S300 RBL Minimal Build"
	@echo "======================"
	@echo "make all           - 构建极简RBL，输出 .elf .bin .hex .dis (无printf)"
	@echo "make s300_image    - 生成 header.bin 与 complete.bin"
	@echo "make analyze_header- 分析header合规性"
	@echo "make dbg           - 启动GDB调试 (:3333)"
	@echo "make clean         - 清理构建产物"