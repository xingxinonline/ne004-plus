Q				?= @
ROOT_DIR		=$(shell pwd)
#exebin			=$(shell $(ROOT_DIR)/tool/bootgen $(ROOT_DIR)/bin/flash.bin bin/armapp/cortexm4_ram_rw.bin $(ROOT_DIR)/bin/riscv64app/riscv64_ram_rw.bin)
ARM_PROJ_NAME	?=default
RISCV_PROJ_NAME	?=default
SUBDIRS = riscv64_default \
		  cortexm4_default \


_ARM_DEBUG ?=1
_RISCV_DEBUG ?=1
OS_NAME := $(shell uname -s)
ifeq ($(OS),Windows_NT)
CROSS_ARM_PATH ?="/D/Home/Program/eclipse/tools/crosstools/gcc-arm-none-eabi/bin/"
CROSS_RISCV_PATH ?="/D/Home/Program/eclipse/tools/crosstools/riscv64-unknown-elf/bin/"
WIN_EXT	?=.exe
else
ifeq ($(OS_NAME),Linux) 
CROSS_ARM_PATH ?=
CROSS_RISCV_PATH ?=
WIN_EXT	?=
endif

endif


export CROSS_ARM_PATH CROSS_RISCV_PATH OS_NAME WIN_EXT _ARM_DEBUG _RISCV_DEBUG

all:
	$(Q)for exefile in $(SUBDIRS) ; do \
		$(MAKE) -C $$exefile ENCPY=0 -s; \
	done
#	$(Q)echo "create flash.bin"
#	$(Q)./tool/bootgen$(WIN_EXT) bin/flash.bin cortexm4_$(ARM_PROJ_NAME)/out/cortexm4_$(ARM_PROJ_NAME).bin riscv64_$(RISCV_PROJ_NAME)/out/riscv64_$(RISCV_PROJ_NAME).bin
#	$(Q)echo "create flashswp.bin"
#	$(Q)./tool/swapbin$(WIN_EXT) bin/flashswp.bin bin/flash.bin
#	$(Q)echo "boot done"
	$(Q)./tool/bootgen$(WIN_EXT).sh cortexm4_$(ARM_PROJ_NAME) riscv64_$(RISCV_PROJ_NAME)
	$(Q)for exefile in $(SUBDIRS) ; do \
		$(MAKE) -C $$exefile inf ENCPY=0 -s; \
	done

proflash:
	./tool/ProFlash$(WIN_EXT) w bin/flash.bin 0x00000000

proflashbug:
	./tool/ProFlash$(WIN_EXT) w bin/flashswp.bin 0x00000000

#make eraseflash address=0x00000000 size=0x100
eraseflash:
	./tool/ProFlash$(WIN_EXT) e $(address) $(size)

#make readflash filename=fw.bin address=0x00000000 size=0x100
readflash:
	./tool/ProFlash$(WIN_EXT) r $(filename) $(address) $(size)

#make writeflash filename=fw.bin address=0x00000000 size=0x100
writeflash:
	./tool/ProFlash$(WIN_EXT) w $(filename) $(address) $(size)

clean:
	$(Q)for exefile in $(SUBDIRS) ; do \
		$(MAKE) -C $$exefile clean -s; \
	done


distclean:
	$(Q)for exefile in $(SUBDIRS) ; do \
		$(MAKE) -C $$exefile distclean -s; \
	done
	$(Q)-rm -rf bin/*

