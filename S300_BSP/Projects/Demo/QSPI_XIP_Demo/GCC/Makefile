TARGET      := s300_demo_qspi_xip
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
ROOT        := $(abspath $(CURDIR)/../../../..)
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
LD_SCRIPT   := $(ROOT)/ld/sram.ld

# 支持不同版本的XIP demo
# 使用方法: make XIP_VERSION=advanced
XIP_VERSION ?= basic
ifeq ($(XIP_VERSION),advanced)
    MAIN_SOURCE := $(PROJECT_DIR)/Src/main_advanced.c
    TARGET := s300_demo_qspi_xip_advanced
else
    MAIN_SOURCE := $(PROJECT_DIR)/Src/main.c
    TARGET := s300_demo_qspi_xip_basic
endif

CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-O2 -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra \
	-I$(DEVICE_DIR)/Include -I$(ROOT)/CMSIS/Core/Include \
	-I$(ROOT)/Boards/generic_evb \
	-I$(ROOT)/Drivers/SoC/RCC/Include \
	-I$(ROOT)/Drivers/SoC/GPIO/Include \
	-I$(ROOT)/Drivers/SoC/UART/Include \
	-I$(ROOT)/Drivers/SoC/QSPI/Include \
	-I$(ROOT)/Drivers/External/W25Qxx/Include
ASFLAGS := $(CFLAGS)

LDFLAGS := -T$(LD_SCRIPT) -nostartfiles -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
   -Wl,-u,_printf_float -Wl,-u,_scanf_float -Wl,-u,_printf_long_long -Wl,-u,_scanf_long_long

SRCS := \
	$(DEVICE_DIR)/Source/GCC/startup_S300.S \
	$(DEVICE_DIR)/Source/system_S300.c \
	$(MAIN_SOURCE) \
	$(ROOT)/Boards/generic_evb/board.c \
	$(ROOT)/Boards/generic_evb/retarget.c \
	$(ROOT)/Boards/generic_evb/syscalls.c \
	$(ROOT)/Drivers/SoC/RCC/Source/rcc.c \
	$(ROOT)/Drivers/SoC/GPIO/Source/gpio.c \
	$(ROOT)/Drivers/SoC/UART/Source/uart.c \
	$(ROOT)/Drivers/SoC/QSPI/Source/qspi_cadence.c \
	$(ROOT)/Drivers/External/W25Qxx/Source/w25qxx.c

SRCS_REL := $(patsubst $(ROOT)/%,%,$(SRCS))
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS_REL))
OBJS := $(patsubst %.c,%.o,$(OBJS))
OBJS := $(patsubst %.S,%.o,$(OBJS))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: $(ROOT)/%.c | prep
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(ROOT)/%.S | prep
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	$(SIZE) $@

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

prep:
	@mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

print:
	@echo "TARGET       = $(TARGET)"
	@echo "BUILD_DIR    = $(BUILD_DIR)"
	@echo "PROJECT_DIR  = $(PROJECT_DIR)"
	@echo "ROOT         = $(ROOT)"
	@echo "DEVICE_DIR   = $(DEVICE_DIR)"
	@echo "LD_SCRIPT    = $(LD_SCRIPT)"
	@echo "XIP_VERSION  = $(XIP_VERSION)"
	@echo "MAIN_SOURCE  = $(MAIN_SOURCE)"
	@echo "SRCS         ="
	@for src in $(SRCS); do echo "  $$src"; done

help:
	@echo "Available targets:"
	@echo "  all      - Build the project"
	@echo "  clean    - Clean build files"
	@echo "  print    - Print build configuration"
	@echo "  dbg      - Launch GDB for debugging"
	@echo "  help     - Show this help"
	@echo ""
	@echo "XIP Demo Versions:"
	@echo "  make XIP_VERSION=basic    - Basic XIP demo (default)"
	@echo "  make XIP_VERSION=advanced - Advanced XIP demo with more features"
	@echo ""
	@echo "Debug examples:"
	@echo "  make XIP_VERSION=basic dbg    - Debug basic XIP demo"
	@echo "  make XIP_VERSION=advanced dbg - Debug advanced XIP demo"

# GDB调试目标
GDB_PORT ?= 3333
dbg: all
	@echo "Launching GDB for $(TARGET)..."
	@ELF_PATH="$(BUILD_DIR)/$(TARGET).elf"; \
	if [ ! -f "$$ELF_PATH" ]; then \
	  echo "ELF file not found: $$ELF_PATH"; exit 2; \
	fi; \
	echo "Using ELF: $$ELF_PATH (GDB_PORT=$(GDB_PORT))"; \
	arm-none-eabi-gdb -q -ex "file $$ELF_PATH" -ex "target extended-remote :$(GDB_PORT)" -x $(ROOT)/gdbinit.common.gdb

.PHONY: all clean prep print help dbg
