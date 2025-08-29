#
# RBL (ROM Bootloader) Makefile
# S300 PiMCHIP 项目
#

# 工具链配置
CROSS_COMPILE = arm-none-eabi-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
OBJDUMP = $(CROSS_COMPILE)objdump
SIZE = $(CROSS_COMPILE)size

# 目录结构
PROJECT_ROOT = ..
SRC_DIR = $(PROJECT_ROOT)/Src
INC_DIR = $(PROJECT_ROOT)/Inc
BUILD_DIR = build
BSP_ROOT = ../../..

# S300 BSP目录
CMSIS_DIR = $(BSP_ROOT)/CMSIS
DEVICE_DIR = $(CMSIS_DIR)/Device/PiMCHIP/S300
DRIVERS_DIR = $(BSP_ROOT)/Drivers
BOARD_DIR = $(BSP_ROOT)/Boards/generic_evb

# 目标文件
TARGET = rbl
ELF_FILE = $(BUILD_DIR)/$(TARGET).elf
BIN_FILE = $(BUILD_DIR)/$(TARGET).bin
HEX_FILE = $(BUILD_DIR)/$(TARGET).hex
MAP_FILE = $(BUILD_DIR)/$(TARGET).map

# 编译选项
MCU = -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard

CFLAGS = $(MCU) \
         -std=c99 \
         -Wall \
         -Wextra \
         -Wno-unused-parameter \
         -fdata-sections \
         -ffunction-sections \
         -g3 \
         -Os

# 调试版本额外选项
DEBUG_CFLAGS = -DDEBUG -O0 -g3

# 链接选项
LDFLAGS = $(MCU) \
          -specs=nano.specs \
          -specs=nosys.specs \
          -T rbl_linker.ld \
          -Wl,--gc-sections \
          -Wl,--print-memory-usage \
          -Wl,-Map=$(MAP_FILE)

# 包含路径
INCLUDES = -I$(INC_DIR) \
           -I$(CMSIS_DIR)/Core/Include \
           -I$(DEVICE_DIR)/Include \
           -I$(DRIVERS_DIR)/SoC/UART/Include \
           -I$(DRIVERS_DIR)/SoC/QSPI/Include \
           -I$(DRIVERS_DIR)/SoC/GPIO/Include \
           -I$(DRIVERS_DIR)/SoC/RCC/Include \
           -I$(BOARD_DIR)

# 预定义宏
DEFINES = -DS300 \
          -DRBL_VERSION_MAJOR=1 \
          -DRBL_VERSION_MINOR=0 \
          -DRBL_VERSION_PATCH=0 \
          -DHSE_CLOCK_HZ=24000000UL \
          -DUSE_STDPERIPH_DRIVER

# 源文件
SOURCES = $(SRC_DIR)/rbl_main.c \
          $(SRC_DIR)/rbl_system.c \
          $(SRC_DIR)/rbl_uart.c \
          $(SRC_DIR)/rbl_qspi.c \
          $(SRC_DIR)/rbl_download.c \
          $(SRC_DIR)/rbl_ymodem.c \
          $(SRC_DIR)/rbl_flash.c \
          $(SRC_DIR)/rbl_crc.c \
          $(SRC_DIR)/startup_s300.c \
          $(DRIVERS_DIR)/SoC/UART/Source/uart.c \
          $(DRIVERS_DIR)/SoC/QSPI/Source/qspi_cadence.c \
          $(DRIVERS_DIR)/SoC/RCC/Source/rcc.c \
          $(DEVICE_DIR)/Source/system_S300.c

# 生成对象文件列表
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/$(SRC_DIR)

# 默认目标
all: $(BIN_FILE) $(HEX_FILE) size

# 调试版本
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: DEFINES += -DRBL_DEBUG=1
debug: all

# 生成ELF文件
$(ELF_FILE): $(OBJECTS) | $(BUILD_DIR)
	@echo "Linking $@"
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# 生成BIN文件
$(BIN_FILE): $(ELF_FILE)
	@echo "Creating $@"
	$(OBJCOPY) -O binary $< $@

# 生成HEX文件
$(HEX_FILE): $(ELF_FILE)
	@echo "Creating $@"
	$(OBJCOPY) -O ihex $< $@

# 编译C文件
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "Compiling $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

# 显示大小信息
size: $(ELF_FILE)
	@echo ""
	@echo "=== Size Information ==="
	$(SIZE) $(ELF_FILE)

# 反汇编
disasm: $(ELF_FILE)
	$(OBJDUMP) -D $< > $(BUILD_DIR)/$(TARGET).dis

# 生成S300头部+RBL组合文件
s300_image: $(BIN_FILE)
	@echo "Generating S300 Header + RBL image..."
	python3 generate_s300_header.py $(BIN_FILE) $(BUILD_DIR)/rbl_header.bin
	python3 merge_s300_image.py $(BUILD_DIR)/rbl_header.bin $(BIN_FILE) $(BUILD_DIR)/s300_rbl_complete.bin
	@echo "S300 complete image: $(BUILD_DIR)/s300_rbl_complete.bin"

# 清理
clean:
	rm -rf $(BUILD_DIR)

# 重新构建
rebuild: clean all

# 帮助信息
help:
	@echo "RBL (ROM Bootloader) Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build RBL binary and hex files"
	@echo "  debug       - Build debug version with extra symbols"
	@echo "  s300_image  - Generate complete S300 Header + RBL image"
	@echo "  size        - Show memory usage"
	@echo "  disasm      - Generate disassembly"
	@echo "  clean       - Remove all build files"
	@echo "  rebuild     - Clean and build"
	@echo "  help        - Show this help"
	@echo ""
	@echo "Build configuration:"
	@echo "  MCU: Cortex-M4 with FPU"
	@echo "  Target: S300 PiMCHIP"
	@echo "  Memory: SRAM execution (256KB)"

# 声明伪目标
.PHONY: all debug clean rebuild size disasm s300_image help

# 自动生成依赖
-include $(OBJECTS:.o=.d)
