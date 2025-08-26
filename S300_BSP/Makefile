## S300 CMSIS BSP build entry (supports multiple Projects/*)
.PHONY: all clean help dbg list new

# Select project (folder name under Projects). Default to Minimal.
PROJECT ?= HelloWorld

# Convenience: discover available project names
RAW_PROJECT_DIRS := $(dir $(wildcard Projects/*/GCC/Makefile) $(wildcard Projects/*/*/GCC/Makefile))
# Strip trailing 'GCC/' and leading 'Projects/' to get friendly project names
PROJECT_DIRS := $(patsubst %/GCC/,%, $(RAW_PROJECT_DIRS))
PROJECTS := $(patsubst Projects/%,%,$(patsubst %/,%,$(PROJECT_DIRS)))

# GDB port can be customized; default OpenOCD/J-Link is :3333
GDB_PORT ?= 3333

all:
	@if [ ! -f "Projects/$(PROJECT)/GCC/Makefile" ]; then \
	  echo "Unknown PROJECT=\"$(PROJECT)\". Use 'make list' to see available projects."; \
	  exit 2; \
	fi; \
	$(MAKE) -C Projects/$(PROJECT)/GCC

clean:
	@if [ ! -f "Projects/$(PROJECT)/GCC/Makefile" ]; then \
	  echo "Unknown PROJECT=\"$(PROJECT)\". Use 'make list' to see available projects."; \
	  exit 2; \
	fi; \
	$(MAKE) -C Projects/$(PROJECT)/GCC clean

help:
	@echo "S300 BSP build system (multiple demos/apps)."
	@echo "Targets:"
	@echo "  make [PROJECT=<name>]                 # build (default PROJECT=$(PROJECT))"
	@echo "  make clean [PROJECT=<name>]           # clean"
	@echo "  make dbg [PROJECT=<name>] [ELF=...] [GDB_PORT=$(GDB_PORT)]  # launch gdb"
	@echo "  make list                    # list available projects"
	@echo "  make new KIND=<HelloWorld> NAME=<ProjectName>   # scaffold new project"
	@echo "Available projects: $(PROJECTS)"

list:
	@echo "Projects: $(PROJECTS)"

new:
	@if [ -z "$(KIND)" ] || [ -z "$(NAME)" ]; then \
	  echo "Usage: make new KIND=<HelloWorld> NAME=<ProjectName>"; exit 2; \
	fi; \
	BASE=Projects/$(KIND)/$(NAME); \
	if [ -e "$$BASE" ]; then echo "Project already exists: $$BASE"; exit 2; fi; \
	mkdir -p "$$BASE/GCC" "$$BASE/Src"; \
	echo "TARGET      := s300_$$(echo $(KIND)_$(NAME) | tr '[:upper:]' '[:lower:]' | tr '/' '_')" > "$$BASE/GCC/Makefile"; \
	echo "BUILD_DIR   := build" >> "$$BASE/GCC/Makefile"; \
	echo "PROJECT_DIR := $$(printf '%s' "$$(cd $$BASE/GCC/.. && pwd)")" >> "$$BASE/GCC/Makefile"; \
	echo "ROOT        := $$(printf '%s' "$$(cd $$BASE/GCC/../.. && pwd)")" >> "$$BASE/GCC/Makefile"; \
	echo "DEVICE_DIR  := $$(printf '%s' "$$(cd $$BASE/GCC/../../.. && pwd)")/CMSIS/Device/PiMCHIP/S300" >> "$$BASE/GCC/Makefile"; \
	echo "LD_SCRIPT   := \\$(ROOT)/ld/sram.ld" >> "$$BASE/GCC/Makefile"; \
	cp Projects/HelloWorld/Src/main.c "$$BASE/Src/main.c"; \
	cat Projects/HelloWorld/GCC/Makefile | sed 's|TARGET      :=.*|TARGET      := s300_$$(echo $(KIND)_$(NAME) | tr '"'\''[:upper:]'"'\'' '"'\''[:lower:]'"'\'' | tr '/' '_')|' >> "$$BASE/GCC/Makefile"; \
	cp Projects/HelloWorld/Src/retarget.c "$$BASE/Src/retarget.c"; \
	printf "#include \"s300.h\"\n#include <stdio.h>\n#ifndef UART_DEBUG_IDX\n#define UART_DEBUG_IDX 3u\n#endif\nstatic void minimal_uart3_gpio_init(void){const uint32_t RCC_BASE=0x4000A000u;const uint32_t APB1_CLK_EN_OFF=0x000Cu;volatile uint32_t*const APB1_CLK_EN=(uint32_t*)(RCC_BASE+APB1_CLK_EN_OFF);*APB1_CLK_EN|=(1u<<8)|(1u<<3);const uint32_t IO_MATRIX_BASE=0x40008000u;volatile uint32_t*const IO_MATRIX_CFG1=(uint32_t*)(IO_MATRIX_BASE+4u);uint32_t v=*IO_MATRIX_CFG1;v&=~((0x3u<<20)|(0x3u<<22));v|=((0x3u<<20)|(0x3u<<22));*IO_MATRIX_CFG1=v;}\nstatic void uart_set_baud(uint32_t base,uint32_t baud){uint32_t clk=SystemCoreClock;if(baud==0u||clk==0u)return;uint32_t denom=baud*16u;uint32_t div=clk/denom;uint32_t rem=clk%denom;uint32_t dlf=(uint32_t)((uint64_t)rem*16u+(denom/2u))/denom;if(dlf>=16u){dlf=0u;div+=1u;}if(div==0u)div=1u;UART_LCRn(base)|=0x80;UART_DLLn(base)=div&0xFFu;UART_DLHn(base)=(div>>8)&0xFFu;UART_LCRn(base)&=~0x80u;UART_DLFn(base)=dlf&0x0Fu;}\nstatic void uart_init_poll(uint32_t idx,uint32_t baud){uint32_t base=UARTn_BASE(idx);UART_IERn(base)=0x0u;UART_FCRn(base)=0x07u;UART_LCRn(base)=0x03u;UART_MCRn(base)=0x00u;uart_set_baud(base,115200u);}\nint main(void){minimal_uart3_gpio_init();uart_init_poll(UART_DEBUG_IDX,115200u);setvbuf(stdout,NULL,_IONBF,0);printf(\"[%s/%s] Hello!\\n\");for(;;){__WFI();}}\n" "$(KIND)" "$(NAME)" > "$$BASE/Src/main.c"; \
	echo "Scaffolded $$BASE"
dbg: all
	@echo "Launching GDB (PROJECT=$(PROJECT))"
	@ELF_PATH="$${ELF}"; \
	if [ -z "$$ELF_PATH" ]; then \
	  ELF_PATH=$$(ls -t Projects/$(PROJECT)/GCC/build/*.elf 2>/dev/null | head -n1); \
	fi; \
	if [ -z "$$ELF_PATH" ]; then \
	  echo "No ELF found (and none provided via ELF=...). Build failed?"; exit 2; \
	fi; \
	echo "Using ELF: $$ELF_PATH (GDB_PORT=$(GDB_PORT))"; \
	arm-none-eabi-gdb -q -ex "file $$ELF_PATH" -ex "target extended-remote :$(GDB_PORT)" -x gdbinit.common.gdb

