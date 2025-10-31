# SPDX-License-Identifier: GPL-2.0-or-later
# S300 Flash Loader Makefile
# Based on OpenOCD STMQSPI loader build system

# Toolchain
CROSS_COMPILE ?= arm-none-eabi-

CC=$(CROSS_COMPILE)gcc
AS=$(CROSS_COMPILE)as
LD=$(CROSS_COMPILE)ld
OBJCOPY=$(CROSS_COMPILE)objcopy
OBJDUMP=$(CROSS_COMPILE)objdump

# Source files
ASM_SRCS = flash_ops.S
C_SRCS = loader_main.c

# Output files
OBJS = $(ASM_SRCS:.S=.o) $(C_SRCS:.c=.o)
TARGET = flash_loader_pic

# Compiler flags for position independent code
CFLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS += -Os -g -Wall -Werror
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -fPIC -fno-common
CFLAGS += -nostdlib -nostartfiles

# Assembler flags
ASFLAGS = -mcpu=cortex-m4 -mthumb -g

# Linker flags - no fixed base address for PIC
LDFLAGS = -T flash_ops.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -nostdlib

# Targets
.PHONY: all clean info

all: $(TARGET).elf $(TARGET).bin flash_ops.inc flash_ops.lst

# Assemble with listing
%.o: %.S
	$(CC) $(CFLAGS) $(ASFLAGS) -Wa,-adhlmn -o $@ -c $< > $(@:.o=.lst)

# Compile C source
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET).elf: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@
	$(OBJDUMP) -D $@ > $(TARGET).dis

# Extract binary
$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Special: build just the PIC assembly part
flash_ops.o: flash_ops.S
	$(CC) $(CFLAGS) $(ASFLAGS) -Wa,-adhlmn -o $@ -c $< > flash_ops.lst

flash_ops.elf: flash_ops.o
	$(LD) -s --defsym=_start=0 -o $@ $<

flash_ops.bin: flash_ops.elf
	$(OBJCOPY) -S -O binary $< $@

# Convert to C include file (like OpenOCD)
flash_ops.inc: flash_ops.bin
	@echo "/* Auto-generated from flash_ops.bin */" > $@
	@echo "#include <stdint.h>" >> $@
	@echo "const uint8_t flash_ops_code[] = {" >> $@
	@hexdump -v -e '16/1 "0x%02x, " "\n"' $< | sed 's/, $$//' >> $@
	@echo "};" >> $@
	@wc -c < $< | xargs -I {} echo "const uint32_t flash_ops_code_size = {}U;" >> $@
	@echo "Generated $@ from $< ("`wc -c < $<`" bytes)"

# Disassembly
flash_ops.lst: flash_ops.elf
	$(OBJDUMP) -d $< > $@

# Info
info:
	@echo "Flash Loader PIC Build"
	@echo "======================"
	@echo "Toolchain: $(CROSS_COMPILE)"
	@echo "Sources:"
	@echo "  ASM: $(ASM_SRCS)"
	@echo "  C:   $(C_SRCS)"
	@echo ""
	@echo "Key flags:"
	@echo "  -fPIC: Position Independent Code"
	@echo "  -defsym=_start=0: Link at offset 0 (relocatable)"
	@echo ""

# Clean
clean:
	rm -f *.o *.elf *.bin *.inc *.lst *.dis *.map
