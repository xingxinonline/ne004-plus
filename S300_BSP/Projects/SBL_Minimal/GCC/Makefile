TARGET      := s300_sbl_minimal
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
ROOT        := $(PROJECT_DIR)/../..
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
# SBL使用Flash链接脚本，运行在0x00100000
LD_SCRIPT   := $(PROJECT_DIR)/ld/flash.ld

# 目标部署目录
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
	   -I$(PROJECT_DIR)/Inc

ASFLAGS := $(CFLAGS)

# 最小链接选项
LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	   -specs=nosys.specs

# 最简SBL：startup + sbl_minimal.c
SRCS := \
  $(DEVICE_DIR)/Source/GCC/startup_S300.S \
  $(PROJECT_DIR)/Src/sbl_minimal.c

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

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(BUILD_DIR)/$(TARGET).dis: $(BUILD_DIR)/$(TARGET).elf
	$(OBJDUMP) -d -S -C -M force-thumb,reg-names-std -j .text $< > $@

# S300 header generation
s300_image: $(BUILD_DIR)/$(TARGET).bin
	@echo "Generating S300 SBL Header + Complete image..."
	python3 ../../RBL/GCC/generate_s300_standard_header.py generate $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET)_header.bin
	python3 ../../RBL/GCC/merge_s300_image.py $(BUILD_DIR)/$(TARGET)_header.bin $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET)_complete.bin || echo "Merge script not found"
	@echo "S300 SBL complete image: $(BUILD_DIR)/$(TARGET)_complete.bin"
	@mkdir -p $(DEST_DIR)
	@cp -f $(BUILD_DIR)/$(TARGET)_complete.bin $(DEST_DIR)/
	@echo "Copied to $(DEST_DIR)/$(TARGET)_complete.bin"

# Build complete S300 system image (Header + RBL + SBL)
system_image: $(BUILD_DIR)/$(TARGET).bin
	@echo "Building complete S300 system image..."
	python3 ../tools/build_system_image.py \
		../../RBL/GCC/build/s300_rbl_simple_complete.bin \
		$(BUILD_DIR)/$(TARGET).bin \
		$(BUILD_DIR)/s300_system_complete.bin
	@mkdir -p $(DEST_DIR)
	@cp -f $(BUILD_DIR)/s300_system_complete.bin $(DEST_DIR)/
	@echo "✅ Complete system image: $(BUILD_DIR)/s300_system_complete.bin"
	@echo "Copied to $(DEST_DIR)/s300_system_complete.bin"

# Analyze system image
analyze_system: $(BUILD_DIR)/s300_system_complete.bin
	@echo "Analyzing complete S300 system image..."
	python3 ../../RBL/GCC/analyze_s300_header.py $(BUILD_DIR)/s300_system_complete.bin

install: s300_image
	@true

analyze_header: $(BUILD_DIR)/$(TARGET)_header.bin
	@echo "Analyzing S300 SBL header format..."
	python3 ../../RBL/GCC/analyze_s300_header.py $(BUILD_DIR)/$(TARGET)_header.bin

# GDB调试
dbg: $(BUILD_DIR)/$(TARGET).elf
	@echo "Starting GDB for SBL (expects server at :3333)..."
	arm-none-eabi-gdb -ex "file $(BUILD_DIR)/$(TARGET).elf" -x $(ROOT)/gdbinit.common.gdb

prep:
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean prep s300_image system_image analyze_system analyze_header help dbg install

help:
	@echo "S300 SBL Minimal Build"
	@echo "======================"
	@echo "make all              - 构建最简SBL，输出 .elf .bin .hex .dis"
	@echo "make s300_image       - 生成 header.bin 与 complete.bin"
	@echo "make complete_image   - 构建完整系统镜像 (Header+RBL+SBL)"
	@echo "make analyze_complete - 分析完整系统镜像结构"
	@echo "make analyze_header   - 分析header合规性"
	@echo "make dbg              - 启动GDB调试 (:3333)"
	@echo "make clean            - 清理构建产物"
	@echo ""
	@echo "SBL运行地址: 0x80010000 (QSPI Flash XIP)"
	@echo "用于测试RBL跳转功能是否正常"
