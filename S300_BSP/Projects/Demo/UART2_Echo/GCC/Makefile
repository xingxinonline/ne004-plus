TARGET      := s300_demo_uart2_echo
BUILD_DIR   := build
PROJECT_DIR := $(CURDIR)/..
# 仓库根目录 S300_BSP
ROOT        := $(abspath $(CURDIR)/../../../..)
DEVICE_DIR  := $(ROOT)/CMSIS/Device/PiMCHIP/S300
LD_SCRIPT   := $(ROOT)/ld/sram.ld

CC      := arm-none-eabi-gcc
AS      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
OBJDUMP := arm-none-eabi-objdump
SIZE    := arm-none-eabi-size

# 使用自定义配方前缀，避免必须以 TAB 开头
.RECIPEPREFIX := >

CFLAGS  := -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
   -O2 -ffunction-sections -fdata-sections -fno-builtin -Wall -Wextra \
   -I$(DEVICE_DIR)/Include -I$(ROOT)/CMSIS/Core/Include \
   -I$(ROOT)/Boards/generic_evb \
   -I$(ROOT)/Drivers/SoC/RCC/Include -I$(ROOT)/Drivers/SoC/GPIO/Include -I$(ROOT)/Drivers/SoC/UART/Include \
   -I$(ROOT)/Drivers/SoC/I2CSoft/Include
ASFLAGS := $(CFLAGS)

# Link with newlib nano and enable float printf support
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
   $(ROOT)/Drivers/SoC/I2CSoft/Source/i2c_soft.c

# 将绝对路径的源码列表转换为以 ROOT 为基准的相对路径，
# 以便在 build 目录下保留源码的目录结构
SRCS_REL := $(patsubst $(ROOT)/%,%,$(SRCS))

# 生成对象文件列表：形如 build/<relative path>.o
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
