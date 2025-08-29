# App Ymodem OTA Makefile

# 项目配置
PROJECT_NAME = app_ymodem_ota
TARGET = $(PROJECT_NAME)

# 构建目录
BUILD_DIR = build
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

# 工具链配置
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
LD = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE = $(PREFIX)size
GDB = $(PREFIX)gdb

# MCU配置
MCU = cortex-m4
FLOAT_ABI = hard
FPU = fpv4-sp-d16

# 编译标志
CFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -Wall -Wextra -Wshadow -Wundef -Wformat=2 -Wno-unused-parameter
CFLAGS += -std=c99 -Os
CFLAGS += -g3 -DUSE_FULL_ASSERT

# 包含路径
INCLUDES = -I../Inc
INCLUDES += -I../../../Include
INCLUDES += -I../../../CMSIS/Core/Include
INCLUDES += -I../../../CMSIS/Device/PiMCHIP/S300
INCLUDES += -I../../../CMSIS/Device/PiMCHIP/S300/Include
INCLUDES += -I../../../Drivers/SoC/RCC/Include
INCLUDES += -I../../../Drivers/SoC/GPIO/Include
INCLUDES += -I../../../Drivers/SoC/UART/Include
INCLUDES += -I../../../Drivers/SoC/QSPI/Include

# 链接标志
LDFLAGS = -mcpu=$(MCU) -mthumb -mfloat-abi=$(FLOAT_ABI) -mfpu=$(FPU)
LDFLAGS += -specs=nosys.specs -specs=nano.specs
LDFLAGS += -T../GCC/app.ld
LDFLAGS += -Wl,--gc-sections -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map

# 调试版本标志
DEBUG_CFLAGS = -DDEBUG -O0 -g3
RELEASE_CFLAGS = -DNDEBUG -Os -g1

# 源文件
C_SOURCES = ../Src/app_main.c
C_SOURCES += ../Src/ymodem.c
C_SOURCES += ../Src/app_ota.c
C_SOURCES += ../Src/system_s300_app.c

# 汇编文件
ASM_SOURCES = startup_s300_app.s

# 系统和驱动文件
C_SOURCES += ../../../Drivers/SoC/RCC/Source/rcc.c
C_SOURCES += ../../../Drivers/SoC/GPIO/Source/gpio.c
C_SOURCES += ../../../Drivers/SoC/UART/Source/uart.c

# 对象文件
OBJECTS = $(addprefix $(OBJ_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(OBJ_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# 默认目标
all: $(BIN_DIR)/$(TARGET).bin

# 调试版本
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: all

# 发布版本
release: CFLAGS += $(RELEASE_CFLAGS)
release: all

# 创建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 编译规则
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 链接规则
$(BIN_DIR)/$(TARGET).elf: $(OBJECTS) | $(BIN_DIR)
	$(LD) $(OBJECTS) $(LDFLAGS) -o $@
	$(SIZE) $@

# 生成二进制文件
$(BIN_DIR)/$(TARGET).bin: $(BIN_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# 生成十六进制文件
$(BIN_DIR)/$(TARGET).hex: $(BIN_DIR)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

# 反汇编
$(BIN_DIR)/$(TARGET).list: $(BIN_DIR)/$(TARGET).elf
	$(OBJDUMP) -h -S $< > $@

# 扩展目标
hex: $(BIN_DIR)/$(TARGET).hex
list: $(BIN_DIR)/$(TARGET).list

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 深度清理
distclean: clean
	rm -f *.log *.bak *~

# 重建
rebuild: clean all

# 显示信息
info:
	@echo "========================================="
	@echo "  App Ymodem OTA Demo Build Info"
	@echo "========================================="
	@echo "Project: $(PROJECT_NAME)"
	@echo "Target:  $(TARGET)"
	@echo "MCU:     $(MCU)"
	@echo "FPU:     $(FPU)"
	@echo "========================================="
	@echo "C Sources:"
	@$(foreach src,$(C_SOURCES),echo "  $(src)";)
	@echo "========================================="

# 下载固件
flash: $(BIN_DIR)/$(TARGET).bin
	@echo "Flashing $(TARGET).bin..."
	# TODO: 添加具体的下载命令
	@echo "Flash command not implemented yet"

# OpenOCD调试
openocd:
	openocd -f interface/jlink.cfg -f target/stm32f4x.cfg

# GDB调试
gdb: $(BIN_DIR)/$(TARGET).elf
	$(GDB) $<

# 测试
test: $(BIN_DIR)/$(TARGET).bin
	@echo "Running tests..."
	@echo "Binary size:"
	@ls -la $(BIN_DIR)/$(TARGET).bin
	@echo "ELF sections:"
	@$(SIZE) $(BIN_DIR)/$(TARGET).elf
	@echo "Memory usage:"
	@$(OBJDUMP) -h $(BIN_DIR)/$(TARGET).elf

# 验证构建
verify: $(BIN_DIR)/$(TARGET).bin
	@echo "========================================="
	@echo "  Build Verification"
	@echo "========================================="
	@if [ -f $(BIN_DIR)/$(TARGET).bin ]; then \
		echo "✓ Binary file created successfully"; \
		echo "  Size: $$(stat -c%s $(BIN_DIR)/$(TARGET).bin) bytes"; \
	else \
		echo "✗ Binary file not found"; \
		exit 1; \
	fi
	@if [ -f $(BIN_DIR)/$(TARGET).elf ]; then \
		echo "✓ ELF file created successfully"; \
		$(SIZE) $(BIN_DIR)/$(TARGET).elf; \
	else \
		echo "✗ ELF file not found"; \
		exit 1; \
	fi
	@echo "✓ Build verification passed"

# 分析
analyze: $(BIN_DIR)/$(TARGET).elf
	@echo "========================================="
	@echo "  Binary Analysis"
	@echo "========================================="
	@echo "Section sizes:"
	@$(SIZE) -A $(BIN_DIR)/$(TARGET).elf
	@echo ""
	@echo "Memory map:"
	@grep -A 20 "Memory Configuration" $(BUILD_DIR)/$(TARGET).map || true
	@echo ""
	@echo "Largest symbols:"
	@$(OBJDUMP) -t $(BIN_DIR)/$(TARGET).elf | sort -k1 -n | tail -20

# 伪目标
.PHONY: all debug release clean distclean rebuild info flash openocd gdb test verify analyze hex list

# 变量导出（用于调试Makefile）
print-%:
	@echo $* = $($*)
