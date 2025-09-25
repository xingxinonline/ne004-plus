-include include/config.mk
VERSION 			?= 2023
PATCHLEVEL 			?= 03
SUBLEVEL 			?= 28
EXTRAVERSION 		?=

Q					?= @
USE_PWSHELL			?= 0
# OS.S				?= $(CONFIG_S_OPERATING_SYSTEM:"%"=%)
ifeq ($(findstring Window,$(OS)),Window)
ifdef MSYSTEM
# MSYS2 environment
export exe_ext		?=.exe
export host			?= win.msys2
export CLRM			 = $(Q)-rm$(exe_ext) -rf
export CPCMD		 = $(Q)cp$(exe_ext) -a 
export sep 			:= /
else
# PowerShell environment
export exe_ext		?=.exe
export host			?= win
export CLRM			 = $(Q)-rm$(exe_ext) -r
export CPCMD		 = $(Q)cp$(exe_ext) -a
export sep 			:= \\

endif

else #Window
OS_ARCH 		?= $(shell uname -a || echo '')
ifeq ($(findstring MINGW,$(OS_ARCH)),MINGW)
export exe_ext		?=.exe
export host			?= win.msys2
export CLRM			 = $(Q)-rm$(exe_ext) -rf
export CPCMD		 = $(Q)cp$(exe_ext) -a 
export sep 			:= /
else ifeq ($(findstring window,$(OS_ARCH)),window)
export exe_ext		?=.exe
export host			?= win
export CLRM			 = $(Q)-rm$(exe_ext) -r
export CPCMD		 = $(Q)cp$(exe_ext) -a
export sep 			:= \\

else ifeq ($(findstring Linux,$(OS_ARCH)),Linux)
export exe_ext		?=
export host			?= linux
export CLRM			 = $(Q)-rm$(exe_ext) -rf
export CPCMD		 = $(Q)cp$(exe_ext) -a 
export sep 			:= /
else ifeq ($(findstring Darwin,$(OS_ARCH)),Darwin)
ifeq ($(findstring x86_64,$(OS_ARCH)),x86_64)
# Intel Mac
export exe_ext		?=
export host			?= mac.intel
export CLRM			 = $(Q)-rm$(exe_ext) -rf
export CPCMD		 = $(Q)cp$(exe_ext) -a 
export sep 			:= /
else ifeq ($(findstring arm64,$(OS_ARCH)),arm64)
# Apple Silicon (M1/M2)
export exe_ext		?=
export host			?= mac.arm
export CLRM			 = $(Q)-rm$(exe_ext) -rf
export CPCMD		 = $(Q)cp$(exe_ext) -a 
export sep 			:= /
endif
else
export exe_ext		?=.exe
export host			?= win
export CLRM			 = $(Q)-rm$(exe_ext) -r
export CPCMD		 = $(Q)cp$(exe_ext) -a
export sep 			:= \\

endif

endif #Window

ifeq ($(host),win)
SHELL 			:= powershell.exe
endif

# ifeq ($(host),win)
# MAKEFLAGS += -j$(NUMBER_OF_PROCESSORS)
# else
# MAKEFLAGS += -j$(nproc)
# endif

ifdef CONFIG_S_CHIP_NAME
NAME 			?= $(CONFIG_S_CHIP_NAME:"%"=%)
else
NAME 			?= next
endif
ifdef CONFIG_S_PUBLISHER
PUBLISHER 		?= $(CONFIG_S_PUBLISHER:"%"=%)
else
PUBLISHER 		?= kingstudio
endif

ifdef CONFIG_S_COMPANY
COMPANY 		?= $(CONFIG_S_COMPANY:"%"=%)
else
COMPANY 		?= universe
endif

ifdef CONFIG_S_PROJECT_LANGUAGE
export KINGLANGUAGE		?= $(CONFIG_S_PROJECT_LANGUAGE:"%"=%)
else
export KINGLANGUAGE		?= c
endif

ifdef CONFIG_S_PROJECT_MODE
export PROJECT_MODE		?= $(CONFIG_S_PROJECT_MODE:"%"=%)
else
export PROJECT_MODE		?= debug
endif

ifdef CONFIG_S_PROJECT_NAME
project 		?= $(subst -,_,$(CONFIG_S_PROJECT_NAME:"%"=%))
else
project 		?= universe
endif
#sim fpga chip board
ifdef CONFIG_S_DESIGN_PHASE
phase			?= $(subst -,_,$(CONFIG_S_DESIGN_PHASE:"%"=%))
else
phase			?= chip
endif
#Execution environment
#rom rom_ram8k ram ram8k flash flash_ram8k
ifdef CONFIG_S_LINKER_SCRIPT
LINKER_NAME 	 = $(patsubst layout-%.ld,%, $(CONFIG_S_LINKER_SCRIPT:"%"=%))
ifdef CONFIG_B_IMPORT_VERIFY
ifeq ($(CONFIG_B_IMPORT_VERIFY),1)
VERIFY_NAME		=$(CONFIG_S_SELECT_VERIFY:"%"=%)
ee				?= ram
else
SELECT_EE		= 1
endif
else
SELECT_EE		= 1
endif

ifdef SELECT_EE
ee 				?= $(subst -,_,$(if $(filter default, $(LINKER_NAME)),ram1-ram1,$(LINKER_NAME)))
endif
else
ee				?= ram
endif

# ifdef CONFIG_S_EXECUTION_ENVIRONMENT
# export ee		?= $(subst -,_,$(CONFIG_S_EXECUTION_ENVIRONMENT:"%"=%))
# else
# export ee		?= ram
# endif
#float point:soft hard
ifdef CONFIG_S_FLOAT_UNIT
export fpu		?= $(CONFIG_S_FLOAT_UNIT:"%"=%)
else
export fpu		?= soft
endif
ifdef CONFIG_B_DEBUG_PRINTF_LIB
export rdimon	?= $(CONFIG_B_DEBUG_PRINTF_LIB)
else
export rdimon	?= 0
endif

ifdef CONFIG_S_SYSTEM_ARCH
ARCH			?= $(CONFIG_S_SYSTEM_ARCH:"%"=%)
else
ARCH			?= armv7-m
endif
ifdef CONFIG_S_SYSTEM_CORE
CORE			?= $(CONFIG_S_SYSTEM_CORE:"%"=%)
else
CORE			?= cortex-m4
endif

#undefine CONFIG_S_ABI
ifdef CONFIG_S_ABI
ABI				?= $(CONFIG_S_ABI:"%"=%)
else
ABI				?= 
endif
ifdef CONFIG_S_GDB_DEBUG_IP_PORT
ipport			?= $(CONFIG_S_GDB_DEBUG_IP_PORT:"%"=%)
else
ipport			?= localhost:3333
endif
ifdef CONFIG_S_EN_OS
export EN_OS	?= $(CONFIG_S_EN_OS:"%"=%)
else
export EN_OS	?= none
endif
ifdef CONFIG_N_LD_IRAM1_BASE
ISRAM1_TOP 		?= $(shell printf "0x%X" $$(($(CONFIG_N_LD_IRAM1_BASE) + $(CONFIG_N_LD_IRAM1_SIZE))))
else
ISRAM1_TOP		?= 0x20060000
endif

EN_LD_WARN		?= 1

ifdef CONFIG_N_UART_TEMINAL
UART_TEMINAL	?= $(CONFIG_N_UART_TEMINAL)
else
UART_TEMINAL	?= 0
endif

ip 				?= $(word 1, $(subst :, ,$(strip $(ipport))))
port 			?= $(word 2, $(subst :, ,$(strip $(ipport))))
# ip				?=$(shell echo $(ipport) | awk -F':' '{print $$1}')
# port			?=$(shell echo $(ipport) | awk -F':' '{print $$2}')

export VERSION PATCHLEVEL SUBLEVEL EXTRAVERSION NAME Q ARCH ABI
ROOT_PATH		 = $(shell pwd)

ROOT_NAME		?= $(notdir  $(patsubst %$(sep),%,$(patsubst %$(sep),%,$(ROOT_PATH))))
ifdef SELECT_EE
OUT_BIN_NAME	?= $(ROOT_NAME)
else
OUT_BIN_NAME	?= $(subst _,-,$(CORE)-$(VERIFY_NAME))
endif
OUT_NAME		?=out

APP_PATH		?=$(ROOT_PATH)$(sep)app$(sep)
ARCH_PATH		?=$(ROOT_PATH)$(sep)arch$(sep)
DOC_PATH		?=$(ROOT_PATH)$(sep)doc$(sep)
CORE_PATH		?=$(ARCH_PATH)$(sep)core$(sep)
DRIVER_PATH		?=$(ROOT_PATH)$(sep)driver$(sep)
EXTDRV_PATH		?=$(ROOT_PATH)$(sep)externdevice$(sep)
INCLUDE_PATH	?=$(ROOT_PATH)$(sep)include$(sep)
LINK_PATH		?=$(ROOT_PATH)$(sep)link$(sep)

OUT_PATH		:= $(ROOT_PATH)$(sep)$(OUT_NAME)
BIN_NAME		?= $(OUT_PATH)$(sep)$(OUT_BIN_NAME)
ifeq ($(host),win)
TOP_PATH		?= $(shell dirname$(exe_ext) $(ROOT_PATH))$(sep)
else
TOP_PATH		?= $(patsubst %$(sep)$(ROOT_NAME),%,$(ROOT_PATH))$(sep)
endif

ROOT_BIN_PATH	?= $(TOP_PATH)bin$(sep)
BINDIR			?= $(ROOT_BIN_PATH)$(OUT_BIN_NAME)$(sep)$(ee)$(sep)
# ifeq ($(host),win)
# # MKBINDIR		?= $(shell if (!(Test-Path '$(BINDIR)$(OUT_NAME)')) { New-Item -ItemType Directory -Path '$(BINDIR)$(OUT_NAME)' } else { Remove-Item -Recurse -Force '$(BINDIR)$(OUT_NAME)' })
# # MKBINDIR		?= $(shell if (!(Test-Path '$(BINDIR)$(OUT_NAME)')) { New-Item -ItemType Directory -Path '$(BINDIR)$(OUT_NAME)' })
# MKBINDIR		?= $(shell mkdir$(exe_ext) -p $(ROOT_BIN_PATH);mkdir$(exe_ext) -p $(BINDIR)..$(sep);mkdir$(exe_ext) -p $(BINDIR))
# else
# # MKBINDIR		?= $(shell if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR)$(OUT_NAME); else rm -rf $(BINDIR)$(OUT_NAME); fi)
# MKBINDIR		?= $(shell if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR)$(OUT_NAME); fi)
# endif
TAR_NAME		?= $(project)-$(phase)-$(OUT_BIN_NAME)-$(subst _,-,$(ee))-$(shell date +%Y%m%d%H%M%S).tar.gz

ifdef CONFIG_S_VSCODE_PATH
VSCODE_ENV		:= $(CONFIG_S_VSCODE_PATH:"%"=%)c_cpp_properties.json
else
VSCODE_ENV		?= $(TOP_PATH).vscode$(sep)c_cpp_properties.json
endif

ifdef CONFIG_S_TOOL_PATH
TOOL_PATH		:= $(CONFIG_S_TOOL_PATH:"%"=%)
else
TOOL_PATH		?= $(ROOT_PATH)$(sep)tool$(sep)
endif

ifdef CONFIG_S_SCRIPT_PATH
SCRIPT_PATH		?= $(CONFIG_S_SCRIPT_PATH:"%"=%)
else
SCRIPT_PATH		?= $(TOOL_PATH)
endif

ifdef CONFIG_S_SIM_BIN_PATH
CP_SIM_BIN_PATH		:= $(CONFIG_S_SIM_BIN_PATH:"%"=%)
endif

#std nano none
ifdef CONFIG_S_LIB_STD
export LIBMODE			+= $(CONFIG_S_LIB_STD:"%"=%)
else
export LIBMODE			+= none
endif

export ROOT_NAME TOP_PATH TOOL_PATH TOOL_AUX_PATH CORE_NAME ARCH_NAME CORE_DIR BIN_PATH BINDIR OUT_PATH TAR_NAME SCRIPT_PATH


ifdef CONFIG_S_CROSS_COMPILE_PATH
CROSS_COMPILE_PATH		?=$(CONFIG_S_CROSS_COMPILE_PATH:"%"=%)
else
CROSS_COMPILE_PATH		?=
endif

ifdef CONFIG_S_CROSS_COMPILE
export CROSS_COMPILE	?= $(CROSS_COMPILE_PATH)$(CONFIG_S_CROSS_COMPILE:"%"=%)
else
export CROSS_COMPILE	?= $(CROSS_COMPILE_PATH)arm-none-eabi-
endif 


# Make variables (CC, etc...)
AS				= $(CROSS_COMPILE)as
LD				= $(CROSS_COMPILE)ld
CC				= $(CROSS_COMPILE)gcc
CPP				= $(CC) -E
CXX				= $(CROSS_COMPILE)g++
AR				= $(CROSS_COMPILE)ar
NM				= $(CROSS_COMPILE)nm
STRIP			= $(CROSS_COMPILE)strip
OBJCOPY			= $(CROSS_COMPILE)objcopy
OBJDUMP			= $(CROSS_COMPILE)objdump
RANLIB 			= $(CROSS_COMPILE)ranlib
SIZE			= $(CROSS_COMPILE)size
GDB				= $(CROSS_COMPILE)gdb
READELF			= $(CROSS_COMPILE)readelf
STR				= $(CROSS_COMPILE)strings

PYTHON			?= python
PYTHON2			?= python2
PYTHON3			?= python3

GCC_VERSION := $(shell $(CC) -dumpversion)
GCC_GE_14_2 := $(shell expr "$(GCC_VERSION)" '>=' "14.2.1")

ifdef CONFIG_S_PYTHON
PYTHON_RUN		?=$(CONFIG_S_PYTHON:"%"=%)
else
ifeq ($(findstring linux,$(host)),linux)
PYTHON_RUN		?=$(PYTHON3)
else ifeq ($(findstring mac,$(host)),mac)
PYTHON_RUN		?=$(PYTHON3)
else ifeq ($(findstring win.msys2,$(host)),win.msys2)
PYTHON_RUN		?=$(PYTHON3)
else
PYTHON_RUN		?=$(PYTHON)
endif
endif

MENUCFG			?=menuconfig
GENCFG			?=genconfig

DOXYGEN 		?=doxygen

ifeq ($(KINGLANGUAGE),cpp)
export CCX		:= $(CXX)
else ifeq ($(KINGLANGUAGE),c)
export CCX		:= $(CC)
else
endif

#undefine CONFIG_S_CROSS_COMPILE_INCLUDE_PATH
ifdef CONFIG_S_CROSS_COMPILE_INCLUDE_PATH
CROSS_COMPILE_INCLUDE_PATH	:= $(CONFIG_S_CROSS_COMPILE_INCLUDE_PATH:"%"=%)
else
CROSS_COMPILE_INCLUDE_PATH  ?=$(CROSS_COMPILE_PATH)..$(sep)arm-none-eabi$(sep)include$(sep)
endif 

export AS LD CC CPP CXX AR NM STRIP OBJCOPY OBJDUMP RANLIB SIZE GDB READELF STR
export PYTHON PYTHON2 PYTHON3 DOXYGEN
export MAKE 

# PRJ_LIST 		:= $(shell find $(ROOT_PATH) -type d)
EXCLUDE_LISTS	 = doc \
				  .git% \
				  os \
				  out \
				  tool \
				  \

# Windows 和 Unix 不同平台的自动检测和适配
ifeq ($(host),win)
ifeq ($(USE_PWSHELL),1)
# space := $(null) $(null)
# comma := ,
#EXCLUDE_LISTS_WITH_COMMAS := $(subst $(space),$(comma),$(EXCLUDE_LISTS))
# EXCLUDE_LISTS_WITHOUT_TRAILING_COMMA := $(if $(findstring $(comma),$(EXCLUDE_LISTS_WITH_COMMAS)),$(patsubst %$(comma),%,$(EXCLUDE_LISTS_WITH_COMMAS)),$(EXCLUDE_LISTS_WITH_COMMAS))
# EXCLUDE_LISTS_WILDCARD:=$(subst %,*,$(EXCLUDE_LISTS_WITHOUT_TRAILING_COMMA))
# PowerShell的方式处理过滤
# tmp_lists	:= $(shell $(SHELL) -c "Get-ChildItem -Path '$(ROOT_PATH)' -Directory | Select-Object -ExpandProperty FullName")
# tmp_lists	:= $(shell $(SHELL) -c "Get-ChildItem -Path '$(ROOT_PATH)' -Directory -Recurse | Select-Object -ExpandProperty FullName")
tmp_lists 		:= $(shell $(SHELL) -c "echo '$(ROOT_PATH)'; Get-ChildItem -Path '$(ROOT_PATH)' -Directory -Recurse | Select-Object -ExpandProperty FullName")
BUILD_LISTS 	:= $(foreach dir,$(tmp_lists),$(if $(filter $(EXCLUDE_LISTS),$(notdir $(dir))),,$(dir)))
else
FILTER_OPTIONS 	:= $(foreach pattern,$(EXCLUDE_LISTS),-name $(pattern) -prune -o)
FILTER_OPTIONS 	:= $(FILTER_OPTIONS) -type d -print
BUILD_LISTS 	:= $(shell find$(exe_ext) $(ROOT_PATH) $(FILTER_OPTIONS))
endif

else
# 类 Unix 系统的 find 命令方式处理
# 构建 find 命令的过滤选项
FILTER_OPTIONS 	:= $(foreach pattern,$(EXCLUDE_LISTS),-name $(pattern) -prune -o)
FILTER_OPTIONS 	:= $(FILTER_OPTIONS) -type d -print

# 使用 find 命令过滤目录
BUILD_LISTS 	:= $(shell find $(ROOT_PATH) $(FILTER_OPTIONS))
endif
BUILD_LISTS := $(filter-out $(ROOT_PATH), $(BUILD_LISTS))

INCLUDE_FLAGS 	:= $(addprefix -I, $(BUILD_LISTS))
LD_LIB_FLAGS 	:= $(addprefix -L, $(BUILD_LISTS))

#export CC_VERSION_TEXT
FILTER_LIST			= $(OUT_NAME) .git% .vscode% tool% bin

# user config
export DEFFLAGS	:=
export CPUFLAGS	:=
CFLAGS			:=
LDFLAGS 		:=
CPPFLAGS		:=
export WARNINGS	:=
ifeq ($(KINGLANGUAGE),cpp)
export CXXFLAGS			:= 
export obj-cppsrc		:=
export obj-cxxsrc		:=
endif
obj-src			:=
obj-include 	:=
obj-asm			:=
objects			:=
obj				:=

# exclude_lists 	 = \
# 				   \

exclude_files := \
				.template.c \
				class_type.c \


ifeq ($(host),win)
ifeq ($(USE_PWSHELL),1)
# tmp_obj := $(shell powershell -Command "Get-ChildItem -Path 'C:\\Home\\Study\\make\\cortex-m4\\arch\\core' -Filter '*.S' | Select-Object -ExpandProperty FullName")
tmp_obj := $(foreach dir, $(BUILD_LISTS), $(shell powershell -Command "Get-ChildItem -Path '$(dir)' -Filter '*.S' | Select-Object -ExpandProperty FullName"))
obj-asm += $(tmp_obj)

tmp_src := $(foreach dir, $(BUILD_LISTS), $(shell powershell -Command "Get-ChildItem -Path '$(dir)' -Filter '*.c' | Select-Object -ExpandProperty FullName"))
obj-src += $(tmp_src)

ifeq ($(KINGLANGUAGE),cpp)
tmp_cppsrc := $(foreach dir, $(BUILD_LISTS), $(shell powershell -Command "Get-ChildItem -Path '$(dir)' -Filter '*.cpp' | Select-Object -ExpandProperty FullName"))
obj-cppsrc += $(tmp_cppsrc)
tmp_cxxsrc := $(foreach dir, $(BUILD_LISTS), $(shell powershell -Command "Get-ChildItem -Path '$(dir)' -Filter '*.cxx' | Select-Object -ExpandProperty FullName"))
obj-cxxsrc += $(tmp_cxxsrc)
endif

tmp_include_obj := $(foreach dir, $(BUILD_LISTS), $(shell powershell -Command "Get-ChildItem -Path '$(dir)' -Filter '*.h' | Select-Object -ExpandProperty FullName"))
obj-include += $(filter-out $(MENU_FILE),$(tmp_include_obj))
else
exclude_pattern := $(foreach file, $(exclude_files), ! -name "$(file)")

# tmp_obj := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find$(exe_ext) $(dir) -type f -name "*.S")))
tmp_obj := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find$(exe_ext) $(dir) -type f -name "*.S" $(exclude_pattern))))
obj-asm += $(tmp_obj)
# tmp_src := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find$(exe_ext) $(dir) -type f -name "*.c")))
tmp_src := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find $(dir) -type f -name "*.c" $(exclude_pattern))))
obj-src += $(tmp_src)
ifeq ($(KINGLANGUAGE),cpp)
# tmp_cppsrc :=$(sort $(foreach dir, $(BUILD_LISTS), $(shell find$(exe_ext) $(dir) -type f -name "*.cpp")))
tmp_cppsrc := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find $(dir) -type f -name "*.cpp" $(exclude_pattern))))
obj-cppsrc += $(tmp_cppsrc)
# tmp_cxxsrc :=$(sort $(foreach dir, $(BUILD_LISTS), $(shell find$(exe_ext) $(dir) -type f -name "*.cxx")))
tmp_cxxsrc := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find $(dir) -type f -name "*.cxx" $(exclude_pattern))))
obj-cxxsrc += $(tmp_cxxsrc)
endif
tmp_include_obj := $(sort $(foreach dir, $(BUILD_LISTS), $(shell find$(exe_ext) $(dir) -type f -name "*.h")))
obj-include += $(filter-out $(MENU_FILE),$(tmp_include_obj))

endif

else
#objects := $(patsubst %.c,%.o,$(wildcard *.c))
# tmp_obj := $(foreach dir, $(BUILD_LISTS), $(wildcard $(dir)$(sep)*.S))
tmp_obj := $(foreach dir, $(BUILD_LISTS), $(filter-out $(foreach file, $(exclude_files), $(dir)$(sep)$(file)), $(wildcard $(dir)$(sep)*.S)))
# tmp_obj_exclude := $(addsuffix ,$(addprefix $(shell pwd)$(sep),$(exclude_lists)))
# obj-asm += $(filter-out $(tmp_obj_exclude),$(tmp_obj))
obj-asm += $(tmp_obj)
# obj-asm += $(foreach dir, $(BUILD_LISTS), $(wildcard $(dir)$(sep)*.s))

# tmp_src := $(foreach dir, $(BUILD_LISTS), $(wildcard $(dir)$(sep)*.c))
tmp_src := $(foreach dir, $(BUILD_LISTS), $(filter-out $(foreach file, $(exclude_files), $(dir)$(sep)$(file)), $(wildcard $(dir)$(sep)*.c)))
# tmp_src_exclude := $(addsuffix ,$(addprefix $(shell pwd)$(sep),$(exclude_lists)))
# obj-src += $(filter-out $(tmp_src_exclude),$(tmp_src))
obj-src += $(tmp_src)
ifeq ($(KINGLANGUAGE),cpp)
# tmp_cppsrc := $(foreach dir, $(BUILD_LISTS), $(wildcard $(dir)$(sep)*.cpp))
tmp_cppsrc := $(foreach dir, $(BUILD_LISTS), $(filter-out $(foreach file, $(exclude_files), $(dir)$(sep)$(file)), $(wildcard $(dir)$(sep)*.cpp)))
obj-cppsrc += $(tmp_cppsrc)
# tmp_cxxsrc := $(foreach dir, $(BUILD_LISTS), $(wildcard $(dir)$(sep)*.cxx))
tmp_cxxsrc := $(foreach dir, $(BUILD_LISTS), $(filter-out $(foreach file, $(exclude_files), $(dir)$(sep)$(file)), $(wildcard $(dir)$(sep)*.cxx)))
obj-cxxsrc += $(tmp_cxxsrc)
endif

tmp_include_obj := $(foreach dir, $(BUILD_LISTS), $(wildcard $(dir)$(sep)*.h))
# tmp_include_exclude := $(addsuffix ,$(addprefix $(shell pwd)$(sep),$(exclude_lists)))
# obj-include += $(filter-out $(tmp_include_exclude),$(tmp_include_obj))
obj-include += $(filter-out $(MENU_FILE),$(tmp_include_obj))
#obj-include += $(tmp_include_obj)
endif

objects			+= $(patsubst %.S,%.o,$(obj-asm)) \
				   $(patsubst %.c,%.o,$(obj-src))
ifeq ($(KINGLANGUAGE),cpp)
objects			+= $(patsubst %.cpp,%.o,$(obj-cppsrc)) \
				   $(patsubst %.cxx,%.o,$(obj-cxxsrc))
endif

export CFLAGS LDFLAGS obj-src obj-include obj-asm

CURR_PATHS_LIST	=$(ROOT_PATH)
FILEFLAGS		:= $(addprefix -I,$(ROOT_PATH))
#FILEFLAGS		:= $(foreach dir, $(CURR_PATHS_LIST), -I$(dir))
FILEFLAGS		+= $(INCLUDE_FLAGS)
FILEFLAGS		+= 

DEFFLAGS		+= 
DEFFLAGS		+= 
DEFFLAGS		+= 

ifneq ($(EN_OS),none)
WARNINGS		+= -Wall
WARNINGS		+= -Wno-implicit-function-declaration
WARNINGS		+= -Wno-unused-variable
else
WARNINGS		+= -W -Wall
endif
# WARNINGS		+= -Wno-div-by-zero
WARNINGS		+= 

CPUFLAGS		:= -mcpu=$(CORE) -mthumb-interwork -mthumb
# CPUFLAGS		+= -march=$(ARCH)
ifeq ($(fpu),hard)
DEFFLAGS		+= -D_EN_FPU=1
CPUFLAGS		+= -mfloat-abi=hard -mfpu=fpv4-sp-d16 -mfp16-format=ieee
else
CPUFLAGS		+= -mfloat-abi=soft
endif
ifeq ($(phase),sim)
DEFFLAGS		+= -D_DEBUG_TRACE=1
DEFFLAGS		+= -D_SIM_PHASE=1
else ifeq ($(phase),fpga)
DEFFLAGS		+= -D_DEBUG_TRACE=1
DEFFLAGS		+= -D_FPGA_PHASE=1
else
ifeq ($(CONFIG_B_ENABLE_DBG),1)
DEFFLAGS		+= -D_DEBUG_TRACE=1
endif

ifndef SELECT_EE
DEFFLAGS		+= -D_DEBUG_TRACE=1
endif

ifeq ($(phase),chip)
DEFFLAGS		+= -D_CHIP_PHASE=1
else ifeq ($(phase),board)
DEFFLAGS		+= -D_BOARD_PHASE=1
endif

endif
ifeq ($(KINGLANGUAGE),cpp)
DEFFLAGS		+= -D_LANGUAGE_CPP=1
endif
ifeq ($(KINGLANGUAGE),c)
DEFFLAGS		+= -D_LANGUAGE_C=1
endif

DEFFLAGS		+= -D_ISRAM1_TOP=$(ISRAM1_TOP)
ifneq ($(EN_OS),none)
DEFFLAGS		+= -D_EN_OS_HARDWARE_SUPPORT=1
ifdef CONFIG_N_OS_HEAP_SIZE
DEFFLAGS		+= -D_OS_HEAP_SIZE=$(CONFIG_N_OS_HEAP_SIZE)
endif

endif
ifeq ($(EN_OS),FreeRTOS)
DEFFLAGS		+= -D_EN_OS_FREERTOS=1
export OS_PATH	:=$(ROOT_PATH)$(sep)os$(sep)FreeRTOS

else ifeq ($(EN_OS),RTThread)
DEFFLAGS		+= -D_EN_OS_RTTHREAD=1
export OS_PATH	:=$(ROOT_PATH)$(sep)os$(sep)RTThread

else ifeq ($(EN_OS),MbedOS)
DEFFLAGS		+= -D_EN_OS_MBEDOS=1
export OS_PATH	:=$(ROOT_PATH)$(sep)os$(sep)MbedOS

else ifeq ($(EN_OS),OpenHarmony)
DEFFLAGS		+= -D_EN_OS_OPENHARMONY=1
export OS_PATH	:=$(ROOT_PATH)$(sep)os$(sep)OpenHarmony

else ifeq ($(EN_OS),ucOSIII)
DEFFLAGS		+= -D_EN_OS_UCOSIII=1
export OS_PATH	:=$(ROOT_PATH)$(sep)os$(sep)ucOSIII

else
endif

ifdef CONFIG_RCC_MODULE
DEFFLAGS		+= -D_RCC_MODULE=1
endif

ifdef CONFIG_UART_MODULE
DEFFLAGS		+= -D_UART_MODULE=1
endif

ifdef CONFIG_B_IMPORT_DEMO
ifeq ($(CONFIG_B_IMPORT_DEMO),1)
DEFFLAGS		+= -D_IMPORT_DEMO=1
endif
endif

ifdef CONFIG_B_IMPORT_VERIFY
ifeq ($(CONFIG_B_IMPORT_VERIFY),1)
DEFFLAGS		+= -D_IMPORT_VERIFY=1
endif
endif

#com
export COMFLAGS	:= 
COMFLAGS		+= -nostartfiles 
ifeq ($(KINGLANGUAGE),cpp)
COMFLAGS		+= -ffreestanding
endif
ifeq ($(KINGLANGUAGE),c)
# COMFLAGS		+= -fno-common 
# COMFLAGS		+= -nodefaultlibs 
# COMFLAGS		+= -nolibc 
# COMFLAGS		+= -nostdlib
endif
ifeq ($(PROJECT_MODE),release)
DEFFLAGS		+= 
COMFLAGS		+= -Os
else
COMFLAGS		+= -g -ggdb
DEFFLAGS		+= 

endif
COMFLAGS		+= 
# delete useless function if =1
ifeq ($(CONFIG_B_DEL_USELESS),1)
COMFLAGS		+= -ffunction-sections -fdata-sections
endif
# COMFLAGS		+= -fvisibility=hidden
COMFLAGS		+= 
COMFLAGS		+= 

ifneq ($(EN_OS),none)
-include $(OS_PATH)$(sep)os.Makefile

DEFFLAGS		+= $(OS_DEFFLAGS)

endif

ifeq ($(EN_OS),none)
export OS_EXT_PATH	:= $(ROOT_PATH)$(sep)os$(sep)extension
-include $(OS_EXT_PATH)$(sep)config.mk

DEFFLAGS		+= $(OS_EXT_DEFFLAGS)

endif


CPUFLAGS		+= $(DEFFLAGS)

ASFLAGS			:= $(CPUFLAGS) 
ASFLAGS			+= $(COMFLAGS) 
ASFLAGS			+= $(FILEFLAGS) 
ifdef CONFIG_S_AFLAGS
ASFLAGS			+= $(CONFIG_S_AFLAGS:"%"=%)
endif
ASFLAGS			+= $(WARNINGS)
ASFLAGS			+= 

CFLAGS			:= $(CPUFLAGS)
CFLAGS			+= $(COMFLAGS)
CFLAGS			+= $(FILEFLAGS)
# CFLAGS			+= -std=gnu11
ifeq ($(KINGLANGUAGE),cpp)
# CFLAGS			+= -std=c++14
CFLAGS			+= -std=c11
endif
ifeq ($(KINGLANGUAGE),c)
CFLAGS			+= -std=c11
endif
ifdef CONFIG_S_CFLAGS
CFLAGS			+= $(CONFIG_S_CFLAGS:"%"=%)
endif
ifeq ($(EN_OS),none)
# CFLAGS 			+= --specs=nosys.specs
endif
# ifeq ($(LIBMODE),nano)
# CFLAGS 			+= --specs=nano.specs
# else 
# endif


ifeq ($(rdimon),1)
CFLAGS			+= --specs=rdimon.specs
endif
CFLAGS			+= $(WARNINGS)
CFLAGS			+= 

ifeq ($(KINGLANGUAGE),cpp)
CXXFLAGS		:= $(CPUFLAGS)
CXXFLAGS		+= $(COMFLAGS)
CXXFLAGS		+= $(FILEFLAGS)
CXXFLAGS		+= -std=c++14
ifdef CONFIG_S_CPPFLAGS
CXXFLAGS		+= $(CONFIG_S_CPPFLAGS:"%"=%)
endif

ifeq ($(CONFIG_B_CPP_EXC_RTTI),1)
CXXFLAGS		+= -frtti
CXXFLAGS		+= -fexceptions
else
CXXFLAGS		+= -fno-rtti
CXXFLAGS		+= -fno-exceptions
endif

# CXXFLAGS		+= -fno-use-cxa-atexit
# CXXFLAGS 		+= -fno-threadsafe-statics
CXXFLAGS		+= -Wall -Wextra
ifeq ($(EN_OS),none)
# CXXFLAGS 		+= --specs=nosys.specs
endif
# ifeq ($(LIBMODE),nano)
# CXXFLAGS 		+= --specs=nano.specs
# else 
# endif
ifeq ($(rdimon),1)
CXXFLAGS		+= --specs=rdimon.specs
endif
CXXFLAGS		+= $(WARNINGS)
CXXFLAGS		+= 

endif #KINGLANGUAGE

MAPFILE 		:= -Wl,-Map=$(BIN_NAME).map
LDFLAGS			:= $(CPUFLAGS) 
LDFLAGS			+= $(COMFLAGS) 
LDFLAGS			+= $(FILEFLAGS) 
LDFLAGS			+= $(MAPFILE)
ifeq ($(KINGLANGUAGE),c)
# LDFLAGS			+= -static
# LDFLAGS			+= -nostdlib
endif
LDFLAGS			+= $(addprefix -L,$(ROOT_PATH))
LDFLAGS			+= $(LD_LIB_FLAGS)
ifdef CONFIG_S_LDFLAGS
LDFLAGS			+= $(CONFIG_S_LDFLAGS:"%"=%)
else
endif
ifeq ($(EN_OS),none)
LDFLAGS			+= --specs=nosys.specs
endif
ifeq ($(LIBMODE),nano)
LDFLAGS			+= --specs=nano.specs
else 
endif

ifeq ($(EN_LD_WARN),1)
ifeq ($(GCC_GE_14_2),1)
LDFLAGS			+= -Wl,--no-warn-rwx-segments
endif
endif

# delete useless function if =1
ifeq ($(CONFIG_B_DEL_USELESS),1)
# LDFLAGS			+= -Wl,--gc-sections
LDFLAGS			+= -Wl,--gc-sections
endif
ifeq ($(LIBMODE),nano)
# LDFLAGS			+= -specs=nosys.specs -specs=nano.specs
POSTFIX_LIB_MODE  =_nano
else 
endif
ifeq ($(CONFIG_B_PRINT_FLOAT),1)
LDFLAGS			+= -u _printf_float
endif
ifeq ($(CONFIG_B_SCANF_FLOAT),1)
LDFLAGS			+= -u _scanf_float
endif

export LDLIBS	:=

ifeq ($(KINGLANGUAGE),c)
LDLIBS			+= -lc$(POSTFIX_LIB_MODE)
LDLIBS			+= -lm
LDLIBS			+= -lgcc
LDLIBS			+= -lnosys
ifeq ($(rdimon),1)
LDLIBS			+= -lrdimon
endif
endif
ifeq ($(KINGLANGUAGE),cpp)
# LDLIBS			+= -lstdc++$(POSTFIX_LIB_MODE)
endif

ifneq ($(EN_OS),none)

objects			+= $(obj-os)
ASFLAGS			+= $(OS_AFLAGS)
ifeq ($(KINGLANGUAGE),cpp)
CXXFLAGS		+= $(OS_CFLAGS) $(OS_INCLUDE_PATH)
else
CFLAGS			+= $(OS_CFLAGS) $(OS_INCLUDE_PATH)
endif
LDFLAGS			+= $(OS_INCLUDE_PATH)

endif

ifeq ($(EN_OS),none)

objects			+= $(obj-os-ext)

ASFLAGS			+= $(OS_EXT_AFLAGS)
ifeq ($(KINGLANGUAGE),cpp)
CXXFLAGS		+= $(OS_EXT_CFLAGS)
else
CFLAGS			+= $(OS_EXT_CFLAGS)
endif
LDFLAGS			+= $(OS_EXT_INCLUDE_PATH)

endif

# ifneq ($(host),win)
# ifeq ($(fpu),soft)
# ifeq ($(CORE),cortex-m4)
# LDFLAGS			+= -L$(shell dirname `$(CCX) $(c_flags) -print-libgcc-file-name`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep) \
# 				   -L$(shell dirname `$(CCX) $(c_flags) -print-file-name=libc.a`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep)

# endif

# else ifeq ($(CORE),cortex-m0)
# LDFLAGS			+= -L$(shell dirname `$(CCX) $(c_flags) -print-libgcc-file-name`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep) \
# 				   -L$(shell dirname `$(CCX) $(c_flags) -print-file-name=libc.a`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep)

# endif

# else

# endif #CORE

# else ifeq ($(fpu),hard)
# LDFLAGS			+= -L$(shell dirname `$(CCX) $(c_flags) -print-libgcc-file-name`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep) \
# 				   -L$(shell dirname `$(CCX) $(c_flags) -print-file-name=libc.a`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep)

# endif

# endif #fpu
# else
# ifeq ($(USE_PWSHELL),1)
# ifeq ($(fpu),soft)
# ifeq ($(CORE),cortex-m4)
# LDFLAGS			+= -L$(dir $(shell powershell -Command "$(CCX) -print-libgcc-file-name"))$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep) \
# 				   -L$(dir $(shell powershell -Command "$(CCX) -print-file-name='libc.a'"))$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# # LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep)

# endif

# else ifeq ($(CORE),cortex-m0)
# LDFLAGS			+= -L$(dir $(shell powershell -Command "$(CCX) -print-libgcc-file-name"))$(sep)thumb$(sep)v6-m$(sep)nofp$(sep) \
# 				   -L$(dir $(shell powershell -Command "$(CCX) -print-file-name='libc.a'"))$(sep)thumb$(sep)v6-m$(sep)nofp$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep)

# endif

# else

# endif #CORE

# else ifeq ($(fpu),hard)
# LDFLAGS			+= -L$(dir $(shell powershell -Command "$(CCX) -print-libgcc-file-name"))$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep) \
# 				   -L$(dir $(shell powershell -Command "$(CCX) -print-file-name='libc.a'"))$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep)

# endif

# endif #fpu

# else
# ifeq ($(fpu),soft)
# ifeq ($(CORE),cortex-m4)
# LDFLAGS			+= -L$(shell dirname `$(shell $(CCX) -print-libgcc-file-name)`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep) \
# 				   -L$(shell dirname `$(shell $(CCX) -print-file-name=libc.a)`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v7e-m$(sep)nofp$(sep)

# endif

# else ifeq ($(CORE),cortex-m0)
# LDFLAGS			+= -L$(shell dirname `$(shell $(CCX) -print-libgcc-file-name)`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep) \
# 				   -L$(shell dirname `$(shell $(CCX) -print-file-name=libc.a)`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v6-m$(sep)nofp$(sep)

# endif

# else

# endif #CORE

# else ifeq ($(fpu),hard)
# LDFLAGS			+= -L$(shell dirname `$(shell $(CCX) -print-libgcc-file-name)`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep) \
# 				   -L$(shell dirname `$(shell $(CCX) -print-file-name=libc.a)`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep) \

# ifeq ($(KINGLANGUAGE),cpp)
# LDFLAGS			+= -L$(shell dirname `$(CCX) -print-file-name=libstdc++.a`)$(sep)thumb$(sep)v7e-m+fp$(sep)hard$(sep)

# endif

# endif #fpu

# endif #USE_PWSHELL

# endif #host

ifdef CONFIG_S_LINKER_SCRIPT
LDFILE			:= $(LINK_PATH)$(CONFIG_S_LINKER_SCRIPT:"%"=%)
else
LDFILE			:= $(LINK_PATH)layout-default.ld
endif

ifdef DIS_INF
dependence_list  = clean
endif
dependence_list += $(obj-include)
dependence_list += $(BIN_NAME).elf
dependence_list += $(BIN_NAME).bin
dependence_list += $(BIN_NAME).dua
dependence_list += $(BIN_NAME).vmf
dependence_list += $(BIN_NAME).krv
dependence_list += $(BIN_NAME).rcf
dependence_list += $(BIN_NAME).hex
dependence_list += $(BIN_NAME).dis
dependence_list += $(BIN_NAME).all.dis
dependence_list += $(BIN_NAME).entrypoint
# dependence_list += genflashbin
# dependence_list += gengdbini
dependence_list += cpy
# dependence_list += inf
# dependence_list += $(depends)

ifndef DIS_INF
dependence_list += inf
endif

PHONY += all
all:preload verinf $(dependence_list)

$(objects):$(obj-include)


PHONY += verinf
verinf:
	$(Q)echo "[GCC] $(notdir $(CC))"

$(BIN_NAME).elf:$(objects) $(obj) #$(LDLIBS)
	$(Q)echo "[LD ] $(notdir $@)"
	$(Q)echo "[MAP] $(notdir $(BIN_NAME).map)"
	$(Q)echo "[LK ] $(notdir $(LDFILE))"
ifeq ($(host),win)
#	$(Q)$(shell if (!(Test-Path '$(OUT_PATH)')) { New-Item -ItemType Directory -Path '$(OUT_PATH)' })
	$(Q)$(shell mkdir -p $(OUT_PATH))
else
	$(Q)$(shell if [ ! -d $(OUT_PATH) ]; then mkdir -p $(OUT_PATH); fi)
endif
	$(Q)$(CCX) -L$(INCLUDE_PATH) -T$(LDFILE) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN_NAME).entrypoint: $(BIN_NAME).elf
	$(Q)echo "[EP ] $(notdir $@)"
ifeq ($(findstring linux,$(host)),linux)
	$(Q)$(READELF) -s $^ | grep "start_vector" | sed -n 's/.* \([0-9a-fA-F]\{8\}\) .*start_vector/0x\1/p' > $@
else ifeq ($(findstring mac,$(host)),mac)
	$(Q)$(READELF) -s $^ | grep "start_vector" | sed -n 's/.* \([0-9a-fA-F]\{8\}\) .*start_vector/0x\1/p' > $@
else ifeq ($(findstring win.msys2,$(host)),win.msys2)
	$(Q)$(READELF) -s $^ | grep "start_vector" | sed -n 's/.* \([0-9a-fA-F]\{8\}\) .*start_vector/0x\1/p' > $@
else
	$(Q)$(READELF) -s $^ | grep "start_vector" | sed -n 's/.* \([0-9a-fA-F]\{8\}\) .*start_vector/0x\1/p' > $@
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)getformat.py $@
endif

$(BIN_NAME).bin: $(BIN_NAME).elf
	$(Q)echo "[BIN] $(notdir $@)"
	$(Q)$(OBJCOPY) -O binary $< $@

$(BIN_NAME).dua: $(BIN_NAME).bin
	$(Q)echo "[HW ] $(notdir $@)"
ifeq ($(findstring linux,$(host)),linux)
	$(Q)od -t x4 -An -w4 -v $< > $@
else ifeq ($(findstring mac,$(host)),mac)
	$(Q)od -t x4 -An -v $< | awk '{print $$1}' > $@
else ifeq ($(findstring win.msys2,$(host)),win.msys2)
	$(Q)od -t x4 -An -w4 -v $< > $@
else
	$(Q)od$(exe_ext) -t x4 -An -w4 -v $< > $@
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)getformat.py $@
endif

$(BIN_NAME).vmf: $(BIN_NAME).bin
	$(Q)echo "[HW ] $(notdir $@)"
	$(Q)echo "@00000000" > $@
ifeq ($(findstring linux,$(host)),linux)
	$(Q)od -t x1 -An -w16 -v $< >> $@
else ifeq ($(findstring mac,$(host)),mac)
	$(Q)od -t x1 -An -v $< | awk '{ \
		for (i=1;i<=NF;i++){ \
			printf "%s ",$$i; \
			if (i%16 == 0) printf "\n" \
		} \
	}' >> $@
else ifeq ($(findstring win.msys2,$(host)),win.msys2)
	$(Q)od -t x1 -An -w16 -v $< >> $@
else
	$(Q)od$(exe_ext) -t x1 -An -w16 -v $< >> $@
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)getformat.py $@
endif

$(BIN_NAME).rcf: $(BIN_NAME).bin
#ifdef CONFIG_S_SCRIPT_PATH
	$(Q)echo "[RCF] $(notdir $@)"
	$(Q)echo "@00000000" > $@
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)genrom.py $< $@ --endianness big --capacity 10240
#endif

$(BIN_NAME).krv: $(BIN_NAME).bin
#ifdef CONFIG_S_SCRIPT_PATH
	$(Q)echo "[KRV] $(notdir $@)"
	$(Q)echo "@00000000" > $@
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)bin2rom.py $< $@ little 10240
#endif

$(BIN_NAME).hex: $(BIN_NAME).elf
	$(Q)echo "[HEX] $(notdir $@)"
	$(Q)$(OBJCOPY) -O ihex $< $@

$(BIN_NAME).dis: $(BIN_NAME).elf
	$(Q)echo "[DIS] $(notdir $@)"
	$(Q)$(OBJDUMP) -D $< > $@

$(BIN_NAME).all.dis: $(BIN_NAME).elf
	$(Q)echo "[DIS] $(notdir $@)"
	$(Q)$(OBJDUMP) -D -S $< > $@

# PHONY += gengdbini
# gengdbini:
# 	$(Q)echo "[INI] $(notdir $@)"
# ifeq ($(wildcard $(TOOL_PATH)gengdbinit$(exe_ext)),)
#     $(info $(TOOL_PATH)gengdbinit$(exe_ext) don't exist)
# else
# 	$(Q)$(TOOL_PATH)gengdbinit $(ROOT_PATH)$(sep)gdbinit.ini $(BIN_NAME) $(ip) $(port)
# endif


PHONY += genflashbin
genflashbin:
#	$(Q)$(TOOL_PATH)image$(sep)bootgen_own$(WIN_EXT).sh $(BIN_NAME) $(ROOT_PATH)$(sep)..$(sep)riscv64_default$(sep)out$(sep)riscv64_default

PHONY += inf
inf:
	$(Q)$(SIZE) -G -d $(subst $(ROOT_PATH)$(sep),,$(BIN_NAME).elf)
# ifneq ($(EN_OS),none)
	$(Q)echo system os: $(EN_OS)
ifdef SELECT_EE
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)checklayout.py $(ROOT_BIN_PATH)config.json $(ee)
endif
# endif

PHONY += cpy
cpy:
ifndef DIS_INF
	$(Q)echo "[CPY] '->'  $(patsubst $(TOP_PATH)%$(sep),..$(sep)%,$(BINDIR))"
else
	$(Q)echo "[CPY] '->'  $(patsubst $(TOP_PATH)%$(sep),%,$(BINDIR))"
endif
#	$(Q)$(MKBINDIR)
ifeq ($(host),win)
	$(Q)$(shell mkdir$(exe_ext) -p $(ROOT_BIN_PATH);mkdir$(exe_ext) -p $(BINDIR)..$(sep);mkdir$(exe_ext) -p $(BINDIR))
ifeq ($(USE_PWSHELL),1)
	$(Q)$(shell powershell -Command "Copy-Item -Recurse -Force -Path '.$(sep)$(OUT_NAME)' -Destination '$(patsubst $(TOP_PATH)%$(sep),..$(sep)%,$(BINDIR))'")

ifndef SELECT_EE
ifeq ($(findstring bin,$(CP_SIM_BIN_PATH)),bin)
	$(Q)$(shell powershell -Command "if (Test-Path '$(CP_SIM_BIN_PATH)$(sep)$(OUT_BIN_NAME)') { Remove-Item -Recurse -Force '$(CP_SIM_BIN_PATH)$(sep)$(OUT_BIN_NAME)' }")
	$(Q)$(shell powershell -Command "Copy-Item -Recurse -Force -Path '$(ROOT_BIN_PATH)$(OUT_BIN_NAME)' -Destination '$(CP_SIM_BIN_PATH)$(sep)'")
endif
endif

else
	$(Q)$(shell mkdir -p $(BINDIR)$(OUT_NAME))
	$(CPCMD) .$(sep)$(OUT_NAME) $(BINDIR)

ifndef SELECT_EE
ifeq ($(findstring bin,$(CP_SIM_BIN_PATH)),bin)
	$(Q)$(shell powershell -Command "if (Test-Path '$(CP_SIM_BIN_PATH)$(sep)$(OUT_BIN_NAME)') { Remove-Item -Recurse -Force '$(CP_SIM_BIN_PATH)$(sep)$(OUT_BIN_NAME)' }")
	$(CPCMD) --no-preserve=ownership $(ROOT_BIN_PATH)$(OUT_BIN_NAME) $(CP_SIM_BIN_PATH)$(sep)
endif
endif

endif
else
	$(Q)$(shell if [ ! -d $(BINDIR) ]; then mkdir -p $(BINDIR)$(OUT_NAME); fi)
	$(CPCMD) $(OUT_NAME) $(BINDIR)
ifndef SELECT_EE
ifeq ($(findstring bin,$(CP_SIM_BIN_PATH)),bin)
	$(Q)$(shell if [ -d $(CP_SIM_BIN_PATH)$(sep)$(OUT_BIN_NAME) ]; then rm -rf $(CP_SIM_BIN_PATH)$(sep)$(OUT_BIN_NAME); fi)
	$(CPCMD) $(ROOT_BIN_PATH)$(OUT_BIN_NAME) $(CP_SIM_BIN_PATH)
endif
endif

endif

PHONY += dist
dist:
	$(Q)echo "[TAR] $(TAR_NAME)"
ifeq ($(findstring linux,$(host)),linux)
	$(Q)tar -zcf $(TAR_NAME) $(OUT_NAME)
else ifeq ($(findstring mac,$(host)),mac)
	$(Q)tar -zcf $(TAR_NAME) $(OUT_NAME)
else ifeq ($(findstring win.msys2,$(host)),win.msys2)
	$(Q)tar -zcf $(TAR_NAME) $(OUT_NAME)
else
#TODO:WIN

endif

PHONY += lib
lib:

PHONY += dbg
#-ix file init
#-x file 
dbg:
#	$(Q)echo "[DBG] $(BIN_NAME).elf"
	$(Q)echo "[DBG] $(notdir $(BIN_NAME).elf)"
#	$(Q)$(GDB) -se $(BIN_NAME).elf -ix $(ROOT_PATH)$(sep)gdbinit.ini
	$(Q)$(GDB) -ix $(ROOT_PATH)$(sep)gdbinit.ini

%.o:%.c
ifeq ($(KINGLANGUAGE),cpp)
	$(Q)echo "[CC ] $(notdir $<)"
	$(Q)$(CCX) $(CXXFLAGS) -c $< -o $@
else
	$(Q)echo "[CC ] $(notdir $<)"
	$(Q)$(CCX) $(CFLAGS) -c $< -o $@
endif

ifeq ($(KINGLANGUAGE),cpp)
%.o:%.cpp
	$(Q)echo "[CXX] $(notdir $<)"
	$(Q)$(CCX) $(CXXFLAGS) -c $< -o $@

%.o:%.cxx
	$(Q)echo "[CXX] $(notdir $<)"
	$(Q)$(CCX) $(CXXFLAGS) -c $< -o $@
endif

%.o:%.S
	$(Q)echo "[AS ] $(notdir $<)"
ifeq ($(KINGLANGUAGE),cpp)
	$(Q)$(CC) $(ASFLAGS) -c $< -o $@
else
	$(Q)$(CCX) $(ASFLAGS) -c $< -o $@
endif

%.o:%.s
	$(Q)echo "[AS ] $(notdir $<)"
ifeq ($(KINGLANGUAGE),cpp)
	$(Q)$(CC) $(ASFLAGS) -c $< -o $@
else
	$(Q)$(CCX) $(ASFLAGS) -c $< -o $@
endif


PHONY += clean
clean:
	$(CLRM) $(objects) $(obj) $(rmobjs)

PHONY += distclean
distclean:clean
	$(CLRM) $(BIN_NAME)*.*
	$(CLRM) $(OUT_PATH)
#	$(CLRM) $(ROOT_BIN_PATH)$(OUT_BIN_NAME)
ifneq ($(EN_OS),none)
	$(Q)$(PYTHON_RUN) $(SCRIPT_PATH)rmfile.py $(ROOT_PATH)$(sep)os .o
endif

PHONY += cleantar
cleantar:
	$(CLRM) *.tar.gz

PHONY += getcc
getcc:
	$(Q)echo $(CROSS_COMPILE)


PHONY += addincpath
addincpath:$(CURR_PATHS_LIST)
	$(Q)echo "generate include path"
ifeq ($(host),win)
#TODO:WIN
	$(Q)echo "$(shell echo $(addprefix \",$(addsuffix \",$^)) | sed "s/[[:space:]]/\,\\n/g" > $(VSCODE_ENV))"
else
	$(Q)echo "$(shell echo $(addprefix \",$(addsuffix \",$^)) | sed "s/[[:space:]]/\,\\n/g" > $(VSCODE_ENV))"
endif

PHONY += genincpath
genincpath:$(CURR_PATHS_LIST)
#	$(Q)echo $(CURR_PATHS_LIST)
#	$(Q)echo $(shell whereis $(CC))
	$(Q)echo "generate include path"
#	$(Q)echo $(patsubst $(TOP_PATH)%,$$\{workspaceFolder\}/%,$^)
#	$(Q)echo $(shell echo $(addprefix \",$(addsuffix \",$^)) | sed "s/[[:space:]]/\,\\n/g" > $(VSCODE_ENV))
#	"/home/work/pimchip/tool/gcc-arm-none-eabi-10.3-2021.10/arm-none-eabi/include/**",
	$(Q)echo "{" > $(VSCODE_ENV)
	$(Q)echo -e "\t\"configurations\": [" >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t{" >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\"name\": \"Linux\"," >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\"includePath\": [" >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\t"\"$$\{workspaceFolder\}/**\", >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\t\"/usr/include\"," >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\t\"/usr/include/linux\"," >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\t\"$(CROSS_COMPILE_INCLUDE_PATH)/**\"," >> $(VSCODE_ENV)
	$(Q)echo $(addprefix \",$(addsuffix \",$^)) | sed "s/[[:space:]]/\,\\n/g" | sed "s/^/\t\t\t\t/g" >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t]," >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t\t\"defines\": []" >> $(VSCODE_ENV)
	$(Q)echo -e "\t\t}" >> $(VSCODE_ENV)
	$(Q)echo -e "\t]," >> $(VSCODE_ENV)
	$(Q)echo -e "\t\"version\": 4" >> $(VSCODE_ENV)
	$(Q)echo "}" >> $(VSCODE_ENV)
	$(Q)echo "" >> $(VSCODE_ENV)

PHONY += tst
tst:
	$(Q)echo  $(TAR_NAME)
	$(Q)echo  $(obj-asm)
	$(Q)echo  $(obj-src)
	$(Q)echo  $(obj-include)


PHONY += preload
preload:
ifeq ($(host),win)
#	$(Q)echo on
endif
ifndef SELECT_EE
ifeq ($(host),win)
	$(Q)$(shell powershell -Command "if (Test-Path '$(OUT_PATH)') { Remove-Item -Recurse -Force '$(OUT_PATH)' }")
else
	$(CLRM) $(OUT_PATH)
endif
endif

PHONY += FORCE
FORCE:

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
