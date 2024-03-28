Q				?= @
ARCH			?= armv7-m
ABI				?= 

ROOT_DIR		=$(shell pwd)
LIB_SRC			:=cortexm4_common
COM_DIR			=$(ROOT_DIR)/../cortexm4_common
QEMUPATH		?=/home/work/riscv64/tool/qemu/bin
LDFILEPATH		?= ./Libraries/CMSIS/NE004/NE004PLUS/Source/GCC/
CPUDIRS			:= $(COM_DIR)/cpu
BOOTDIRS		:= $(COM_DIR)/boot
SUBDIRS			:= $(CPUDIRS) 
BUILDDIR		:= $(ROOT_DIR)/build
OUTDIR			:= $(ROOT_DIR)/out
CPYDIR			= bin/$(shell pwd | sed 's/^\(.*\)[/]//' )/

OUTBIN			?= $(OUTDIR)/$(shell pwd | sed 's/^\(.*\)[/]//' )

ifeq ("$(shell uname -n)","raspberrypi")
ip ?=$(shell hostname -I | cut -c 1-10)

ifeq ("${ip}","192.168.50")
ENCPY			?= 1
else
ENCPY			?= 0
endif

CROSS_COMPILE	?= $(CROSS_ARM_PATH)arm-none-eabi-
else
ifeq ("$(shell whoami)","jzh")
ENCPY			?= 1
else
ENCPY			?= 0
endif
CROSS_COMPILE	?= $(CROSS_ARM_PATH)arm-none-eabi-
endif

BINDIR			?=$(ROOT_DIR)/../bin/$(shell pwd | sed 's/^\(.*\)[/]//' )/
MKBINDIR		?= $(shell if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR); else rm -rf $(BINDIR); fi)

# Make variables (CC, etc...)
AS				= $(CROSS_COMPILE)as
LD				= $(CROSS_COMPILE)ld
CC				= $(CROSS_COMPILE)gcc
CPP				= $(CC) -E
AR				= $(CROSS_COMPILE)ar
NM				= $(CROSS_COMPILE)nm
STRIP			= $(CROSS_COMPILE)strip
OBJCOPY			= $(CROSS_COMPILE)objcopy
OBJDUMP			= $(CROSS_COMPILE)objdump
RANLIB 			= $(CROSS_COMPILE)RANLIB
SIZE			= $(CROSS_COMPILE)size
GDB				= $(CROSS_COMPILE)gdb
READELF			= $(CROSS_COMPILE)readelf
STR				= $(CROSS_COMPILE)strings

dirs			:= . \
				./Libraries/NE004xx_Driver/ \
				./Libraries/NE004xx_Driver/Source \
				./Libraries/NE004xx_Driver/Include \
				./Libraries/CMSIS \
				./Libraries/CMSIS/NE004/NE004PLUS/ \
				./Libraries/CMSIS/NE004/NE004PLUS/Source/ \
				./Libraries/CMSIS/NE004/NE004PLUS/Include/ \
				./Libraries/CMSIS/NE004/NE004PLUS/Source/GCC/ \



asrcs			:= $(foreach dir, $(dirs), $(wildcard $(dir)/*.S))
csrcs			:= $(foreach dir, $(dirs), $(wildcard $(dir)/*.c))
depends			:= $(csrcs:%.c=%.d)

FILEFLAGS		:= $(foreach dir, $(dirs), -I$(dir))
FILEFLAGS		+= -I$(COM_DIR)/include			   

COMFLAGS		:= -mcpu=cortex-m4 -mthumb-interwork -mthumb \
				-std=gnu11 \
				-fno-common \
				-nostartfiles \
				-nodefaultlibs \
				-nolibc \
				-nostdlib

# COMFLAGS		+= -march=$(ARCH)
ifeq ($(_ARM_DEBUG),1)
COMFLAGS		+= -O0
COMFLAGS		+= -g -ggdb
endif
COMFLAGS		+= -W -Wall

ASFLAGS			:= $(COMFLAGS) 
CFLAGS			:= $(COMFLAGS)
CFLAGS			+= $(FILEFLAGS)

MAPFILE 		:= -Map $(OUTBIN).map
LDFLAGS			:= $(MAPFILE) \
				-static -nostdlib \

LDLIBS			:= -lc -lm -lgcc -lnosys \

LDFLAGS			+= -L$(shell dirname `$(CC) $(c_flags) -print-libgcc-file-name`) \
				   -L$(shell dirname `$(CC) $(c_flags) -print-file-name=libc.a`) \

# delete useless function if =1
DEL_USELESS		?=
ifeq ($(DEL_USELESS),1)
COMFLAGS		+= -ffunction-sections -fdata-sections
LDFLAGS			+= --gc-sections
else
endif

# debug for qemu if =1
QEMU_DEBUG		?=
ifeq ($(QEMU_DEBUG),1)
LDFILE			:= $(LDFILEPATH)/debug.ld
else
LDFILE			:= $(LDFILEPATH)/link.ld
endif


# find .h file from .c
#depends			:= $(sort $(filter %.h, $(shell $(CC) $(FILEFLAGS) -MM $(csrcs))))

objects			:=
obj				:=

objects			+= $(patsubst %.S,%.o,$(asrcs)) \
				   $(patsubst %.c,%.o,$(csrcs))

obj				+= 

rmobjs			:= \
				$(patsubst %.c,%.i,$(csrcs)) \
				$(patsubst %.c,%.s,$(csrcs)) \
				$(patsubst %.c,%.d,$(csrcs))


export ROOT_DIR QEMUPATH LDFILEPATH CPUDIRS SUBDIRS BOOTDIRS OUTBIN
export ARCH ABI CROSS_COMPILE CFLAGS ASFLAGS LDFLAGS LDLIBS
export AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP RANLIB SIZE GDB READELF STR
export MAKE Q obj objects

all: $(OUTBIN).elf $(OUTBIN).bin $(OUTBIN).dis $(OUTBIN).entrypoint genflashbin cpy

$(OUTBIN).elf: $(objects) $(obj) #$(LDLIBS)
	$(Q)echo [LD ]		$(notdir $@)
	$(Q)echo [MAP]		$(notdir $(OUTBIN).map)
	$(Q)$(shell if [ ! -d $(OUTDIR) ]; then mkdir -p $(OUTDIR); fi)
	$(Q)$(LD) -T$(LDFILE) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OUTBIN).entrypoint: $(OUTBIN).elf
	$(Q)echo [EP ]		$(notdir $@)
	$(Q)$(READELF) -h $^ | grep "Entry point address:" | sed 's/ //g'|cut -d':' -f2 | tr -cd "[0-9a-fA-FxX]" > $@

$(OUTBIN).bin: $(OUTBIN).elf
	$(Q)echo [BIN]		$(notdir $@)
	$(Q)$(OBJCOPY) -O binary $< $@

$(OUTBIN).dua: $(OUTBIN).bin
	$(Q)echo [HW ]		$(notdir $@)
	$(Q)od -t x4 -An -w4 -v $< > $@

$(OUTBIN).hex: $(OUTBIN).elf
	$(Q)echo [HEX]		$(notdir $@)
	$(Q)$(OBJCOPY) -O ihex $< $@

$(OUTBIN).dis: $(OUTBIN).elf
	$(Q)echo [DIS]		$(notdir $@)
	$(Q)$(OBJDUMP) -D $< > $@

$(OUTBIN).all.dis: $(OUTBIN).elf
	$(Q)echo [DIS]		$(notdir $@)
	$(Q)$(OBJDUMP) -D -S $< > $@

genflashbin:
	$(Q)../tool/bootgen_own$(WIN_EXT).sh $(OUTBIN) $(ROOT_DIR)/../riscv64_default/out/riscv64_default

inf:
	$(Q)$(SIZE) -G -d $(OUTBIN).elf

cpy:
	$(Q)echo [CPY]		'->' $(BINDIR)
	$(Q)$(MKBINDIR)
	$(Q)cp -a out $(BINDIR)

$(SUBDIRS):
	$(Q)echo $@
	$(MAKE) -C $@
	$(Q)echo $(objects)

%.d:%.c
	$(Q)echo [DEP]		$(notdir $<)
	$(Q)set -e; rm -f $@; \
	echo $@ $(dir $@)$(shell $(CC) $(FILEFLAGS) -MM $<) > $@;
#	$(Q)echo $(sort $(filter %.h, $(shell $(CC) $(FILEFLAGS) -MM $<))) > $@

#-include $(depends)
#  %.d $(file < %.d)
# $(sort $(filter %.h, $(file < $*.d)))
%.o:%.c
	$(Q)echo [CC ]		$(notdir $<)
	$(Q)$(CC) $(CFLAGS) -c $< -o $@
#	$(Q)$(CC) $(CFLAGS) -E $< -o $*.i
#	$(Q)$(CC) $(CFLAGS) -S $*.i -o $*.s
#	$(Q)$(CC) $(CFLAGS) -c $*.s -o $@

# %.i:%.c
# 	$(Q)echo [CC]		$(notdir $@)
# 	$(Q)$(CC) $(CFLAGS) -E $< -o $@

# %.s:%.i
# 	$(Q)echo [CC]		$(notdir $@)
# 	$(Q)$(CC) $(CFLAGS) -S $< -o $@

# %.o:%.s
# 	$(Q)echo [CC]		$(notdir $@)
# 	$(Q)$(CC) $(CFLAGS) -c $< -o $@

%.o:%.S
	$(Q)echo [AS ]		$(notdir $<)
	$(Q)$(CC) $(ASFLAGS) -c $< -o $@


.PHONY: all clean distclean run debug debugasm

clean:
	$(Q)-rm -rf $(objects) $(obj) $(rmobjs)

distclean: clean
	$(Q)-rm -rf $(OUTBIN)*.*

run:
	$(QEMUPATH)/qemu-system-arm -machine lm3s811evb -cpu cortex-m4 -m 512K -semihosting -kernel $(outbin).elf -S -gdb tcp::7890
#	# $(QEMUPATH)/qemu-system-riscv64 -machine virt -cpu rv64 -m 256K -semihosting -kernel $(OUTBIN).elf  -bios none -serial stdio -display none -S -gdb tcp::7890
	$(Q)$(shell pkill qemu*)

debug:
	$(Q)$(shell pkill qemu*)
	$(Q)$(MAKE) run &
	$(GDB) $(OUTBIN).elf -w -ex 'target remote localhost:7890' -ex 'info source' -ex 'layout src' -ex 'layout regs' -ex 'b reset' \
	-ex 'c' -ex 'si'
	$(Q)echo "debug done"
	$(Q)$(shell pkill qemu*)

debugasm:
	$(Q)$(shell pkill qemu*)
	$(Q)$(MAKE) run &
	$(GDB) $(OUTBIN).elf -w -ex 'target remote localhost:7890' -ex 'info source' -ex 'layout asm' -ex 'layout regs' -ex 'b reset' \
	-ex 'c' # -ex 'b main' -ex 'c'
	$(Q)echo "debug done"
	$(Q)$(shell pkill qemu*)


cssh:
	ssh-keygen -t rsa -P "" -f ~/.ssh/id_rsa
	ifdef iptarget
	ssh-copy-id -i ~/.ssh/id_rsa.pub $(iptarget)
	endif

test:
#	$(Q)echo $(csrcs)
#	$(Q)echo $(asrcs)
#	$(Q)echo $(FILEFLAGS)
#	$(Q)echo $(depends)	
	$(Q)echo $(CROSS_COMPILE)
