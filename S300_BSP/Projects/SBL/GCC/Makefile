# SBL项目Makefile - ESP32兼容的二级引导程序

#******************************************************************************
# 项目配置
#******************************************************************************
PROJECT_NAME := SBL
TARGET := sbl
VERSION_MAJOR := 1
VERSION_MINOR := 0
VERSION_PATCH := 0

# 目录配置
PROJECT_ROOT := $(shell dirname $(shell pwd))
BSP_ROOT := $(PROJECT_ROOT)/../..
OUTPUT_DIR := $(shell pwd)/build
OBJ_DIR := $(OUTPUT_DIR)/obj
BIN_DIR := $(OUTPUT_DIR)/bin

# 源文件目录
SRC_DIR := $(PROJECT_ROOT)/Src
INC_DIR := $(PROJECT_ROOT)/Inc
CMSIS_DIR := $(BSP_ROOT)/CMSIS
DEVICE_DIR := $(CMSIS_DIR)/Device/PiMCHIP/S300
DRIVERS_DIR := $(BSP_ROOT)/Drivers
BOARDS_DIR := $(BSP_ROOT)/Boards/generic_evb

#******************************************************************************
# 工具链配置
#******************************************************************************
PREFIX := arm-none-eabi-
CC := $(PREFIX)gcc
AS := $(PREFIX)as
LD := $(PREFIX)ld
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
SIZE := $(PREFIX)size
GDB := $(PREFIX)gdb
OPENOCD := openocd

#******************************************************************************
# 编译器标志
#******************************************************************************
# CPU配置
CPU := -mcpu=cortex-m4
FPU := -mfpu=fpv4-sp-d16
FLOAT_ABI := -mfloat-abi=hard

# 架构标志
ARCH_FLAGS := $(CPU) -mthumb $(FPU) $(FLOAT_ABI)

# C标志
CFLAGS := $(ARCH_FLAGS)
CFLAGS += -std=gnu99
CFLAGS += -Wall -Wextra -Wno-unused-parameter
CFLAGS += -fdata-sections -ffunction-sections
CFLAGS += -fno-common -fno-builtin
CFLAGS += -MMD -MP

# 优化标志
ifeq ($(DEBUG), 1)
    CFLAGS += -Og -g3 -DDEBUG
    OPT_SUFFIX := _debug
else
    CFLAGS += -Os -g1 -DRELEASE
    OPT_SUFFIX := _release
endif

# 预处理器定义
CFLAGS += -DUSE_HAL_DRIVER
CFLAGS += -DS300
CFLAGS += -DARM_MATH_CM4
CFLAGS += -DSBL_VERSION_MAJOR=$(VERSION_MAJOR)
CFLAGS += -DSBL_VERSION_MINOR=$(VERSION_MINOR)
CFLAGS += -DSBL_VERSION_PATCH=$(VERSION_PATCH)
CFLAGS += -DSBL_BUILD_DATE=\"$(shell date +%Y%m%d)\"
CFLAGS += -DSBL_BUILD_TIME=\"$(shell date +%H%M%S)\"

# 汇编标志
ASFLAGS := $(ARCH_FLAGS)
ASFLAGS += -x assembler-with-cpp

# 链接标志
LDFLAGS := $(ARCH_FLAGS)
LDFLAGS += -specs=nano.specs
LDFLAGS += -specs=nosys.specs
LDFLAGS += -static
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage
LDFLAGS += -Wl,-Map=$(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).map

#******************************************************************************
# 包含路径
#******************************************************************************
INCLUDES := -I$(INC_DIR)
INCLUDES += -I$(SRC_DIR)
INCLUDES += -I$(CMSIS_DIR)/Core/Include
INCLUDES += -I$(DEVICE_DIR)/Include
INCLUDES += -I$(DRIVERS_DIR)/SoC/RCC/Include
INCLUDES += -I$(DRIVERS_DIR)/SoC/GPIO/Include
INCLUDES += -I$(DRIVERS_DIR)/SoC/UART/Include
INCLUDES += -I$(DRIVERS_DIR)/SoC/DMA/Include
INCLUDES += -I$(DRIVERS_DIR)/SoC/QSPI/Include
INCLUDES += -I$(BOARDS_DIR)

#******************************************************************************
# 源文件
#******************************************************************************
# SBL源文件
SBL_SOURCES := $(wildcard $(SRC_DIR)/*.c)

# 设备源文件
DEVICE_SOURCES := $(DEVICE_DIR)/Source/system_S300.c

# 驱动源文件
DRIVER_SOURCES := $(DRIVERS_DIR)/SoC/RCC/Source/rcc.c
DRIVER_SOURCES += $(DRIVERS_DIR)/SoC/GPIO/Source/gpio.c
DRIVER_SOURCES += $(DRIVERS_DIR)/SoC/UART/Source/uart.c
DRIVER_SOURCES += $(DRIVERS_DIR)/SoC/DMA/Source/dma.c
DRIVER_SOURCES += $(DRIVERS_DIR)/SoC/QSPI/Source/qspi_cadence.c

# 板级源文件
BOARD_SOURCES := $(BOARDS_DIR)/board.c
BOARD_SOURCES += $(BOARDS_DIR)/retarget.c
BOARD_SOURCES += $(BOARDS_DIR)/syscalls.c

# 启动文件
STARTUP_SOURCES := $(DEVICE_DIR)/Source/GCC/startup_S300.S

# 所有源文件
C_SOURCES := $(SBL_SOURCES) $(DEVICE_SOURCES) $(DRIVER_SOURCES) $(BOARD_SOURCES)
ASM_SOURCES := $(STARTUP_SOURCES)

#******************************************************************************
# 目标文件
#******************************************************************************
C_OBJECTS := $(C_SOURCES:%.c=$(OBJ_DIR)/%.o)
ASM_OBJECTS := $(ASM_SOURCES:%.s=$(OBJ_DIR)/%.o)
OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# 依赖文件
DEPS := $(C_OBJECTS:.o=.d)

#******************************************************************************
# 链接脚本
#******************************************************************************
LDSCRIPT := $(PROJECT_ROOT)/sbl.ld

#******************************************************************************
# 构建目标
#******************************************************************************
.PHONY: all clean debug release flash flash-openocd gdb info test

# 默认目标
all: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).hex

# 调试版本
debug:
	@$(MAKE) DEBUG=1

# 发布版本  
release:
	@$(MAKE) DEBUG=0

# 创建输出目录
$(OUTPUT_DIR) $(OBJ_DIR) $(BIN_DIR):
	@mkdir -p $@

# 创建对象文件目录
$(OBJ_DIR)/%/:
	@mkdir -p $@

# 编译C源文件
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "CC    $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# 编译汇编源文件
$(OBJ_DIR)/%.o: %.s | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	@echo "AS    $<"
	@$(CC) $(ASFLAGS) $(INCLUDES) -c $< -o $@

# 链接ELF文件
$(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf: $(OBJECTS) | $(BIN_DIR)
	@echo "LD    $@"
	@$(CC) $(LDFLAGS) -T$(LDSCRIPT) $(OBJECTS) -o $@
	@echo ""
	@echo "=== 内存使用情况 ==="
	@$(SIZE) $@
	@echo ""

# 生成二进制文件
$(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@
	@echo "生成二进制文件: $@"
	@ls -la $@

# 生成HEX文件
$(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).hex: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf
	@echo "OBJCOPY $@"
	@$(OBJCOPY) -O ihex $< $@

# 生成反汇编文件
$(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).lst: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf
	@echo "OBJDUMP $@"
	@$(OBJDUMP) -h -S $< > $@

#******************************************************************************
# 下载和调试
#******************************************************************************
# OpenOCD配置
OPENOCD_CFG := $(BSP_ROOT)/openocd/s300_jlink.cfg
OPENOCD_FLASH_CMD := "program $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin 0x20000000 verify reset exit"

# 使用OpenOCD下载
flash-openocd: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin
	@echo "使用OpenOCD下载固件..."
	@$(OPENOCD) -f $(OPENOCD_CFG) -c $(OPENOCD_FLASH_CMD)

# 使用JLink下载
flash: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin
	@echo "使用JLink下载固件..."
	@echo "loadbin $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin,0x20000000" > $(OUTPUT_DIR)/jlink_flash.cmd
	@echo "r" >> $(OUTPUT_DIR)/jlink_flash.cmd
	@echo "g" >> $(OUTPUT_DIR)/jlink_flash.cmd
	@echo "qc" >> $(OUTPUT_DIR)/jlink_flash.cmd
	@JLinkExe -device Cortex-M4 -if SWD -speed 4000 -CommanderScript $(OUTPUT_DIR)/jlink_flash.cmd

# GDB调试
gdb: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf
	@echo "启动GDB调试..."
	@$(GDB) -x $(BSP_ROOT)/gdbinit.gdb $<

#******************************************************************************
# 信息和分析
#******************************************************************************
# 显示项目信息
info:
	@echo ""
	@echo "=== SBL项目信息 ==="
	@echo "项目名称: $(PROJECT_NAME)"
	@echo "目标文件: $(TARGET)"
	@echo "版本号: $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)"
	@echo "构建目录: $(OUTPUT_DIR)"
	@echo "工具链: $(PREFIX)"
	@echo ""
	@echo "=== 编译配置 ==="
	@echo "CFLAGS: $(CFLAGS)"
	@echo ""
	@echo "=== 源文件统计 ==="
	@echo "C源文件: $(words $(C_SOURCES)) 个"
	@echo "汇编文件: $(words $(ASM_SOURCES)) 个"
	@echo "头文件: $(shell find $(INC_DIR) -name "*.h" | wc -l) 个"
	@echo ""

# 代码统计
stats:
	@echo ""
	@echo "=== 代码统计 ==="
	@echo "SBL源代码:"
	@wc -l $(SBL_SOURCES) $(INC_DIR)/*.h | tail -1
	@echo ""

# 反汇编分析
disasm: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).lst
	@echo "反汇编文件已生成: $<"

# 符号表分析
symbols: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf
	@echo "=== 符号表分析 ==="
	@$(OBJDUMP) -t $< | grep -E "(sbl_|main)" | sort

#******************************************************************************
# 测试和验证
#******************************************************************************
# 基本测试
test: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin
	@echo ""
	@echo "=== SBL基本测试 ==="
	@echo "1. 检查二进制文件大小..."
	@SIZE_BYTES=$$(stat -c%s $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin); \
	if [ $$SIZE_BYTES -gt 65536 ]; then \
		echo "错误: 二进制文件过大 ($$SIZE_BYTES bytes > 64KB)"; \
		exit 1; \
	else \
		echo "✓ 二进制文件大小正常: $$SIZE_BYTES bytes"; \
	fi
	@echo ""
	@echo "2. 检查ELF文件结构..."
	@$(OBJDUMP) -h $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf | grep -E "\.(text|data|bss|rodata)"
	@echo ""
	@echo "3. 检查栈和堆配置..."
	@$(OBJDUMP) -t $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).elf | grep -E "(_stack|_heap)"
	@echo ""
	@echo "✓ SBL基本测试通过"

# 验证构建
verify: test
	@echo ""
	@echo "=== SBL验证完成 ==="
	@echo "固件文件: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).bin"
	@echo "映射文件: $(BIN_DIR)/$(TARGET)$(OPT_SUFFIX).map"
	@echo "可以使用 'make flash' 下载到目标板"
	@echo ""

#******************************************************************************
# 清理
#******************************************************************************
clean:
	@echo "清理构建文件..."
	@rm -rf $(OUTPUT_DIR)
	@echo "清理完成"

# 深度清理
distclean: clean
	@echo "深度清理..."
	@find . -name "*.tmp" -delete
	@find . -name "*.bak" -delete
	@echo "深度清理完成"

#******************************************************************************
# 帮助信息
#******************************************************************************
help:
	@echo ""
	@echo "SBL项目构建系统"
	@echo ""
	@echo "可用目标:"
	@echo "  all          - 构建项目 (默认)"
	@echo "  debug        - 构建调试版本"
	@echo "  release      - 构建发布版本"
	@echo "  clean        - 清理构建文件"
	@echo "  distclean    - 深度清理"
	@echo "  flash        - 使用JLink下载固件"
	@echo "  flash-openocd- 使用OpenOCD下载固件"
	@echo "  gdb          - 启动GDB调试"
	@echo "  info         - 显示项目信息"
	@echo "  stats        - 代码统计"
	@echo "  test         - 运行基本测试"
	@echo "  verify       - 验证构建"
	@echo "  help         - 显示此帮助"
	@echo ""
	@echo "示例:"
	@echo "  make debug   - 构建调试版本"
	@echo "  make flash   - 下载固件到目标板"
	@echo "  make clean   - 清理构建文件"
	@echo ""

# 包含依赖文件
-include $(DEPS)
