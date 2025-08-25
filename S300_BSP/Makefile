## Top-level Makefile router for S300_BSP examples
# Usage:
#   make list                       # 列出可用示例名（CASE）
#   make CASE=UART_HelloWorld       # 构建指定示例
#   make dbg CASE=DMA_TransferInterrupt  # 进入 GDB 调试指定示例
#   make clean CASE=DMA_RamToRam    # 清理指定示例
#   make clean-all                  # 清理全部示例构建产物

.PHONY: all build dbg clean clean-all list help

# 自动收集所有带 GCC/Makefile 的示例目录名称作为 CASE 列表
EXAMPLES := $(shell ls -1d Projects/Examples/*/GCC 2>/dev/null | sed -E 's#.*/Examples/([^/]+)/GCC#\1#' | sort)

# 目标示例目录
CASE ?=
EXAMPLE_DIR := Projects/Examples/$(CASE)/GCC
EXAMPLE_MK  := $(EXAMPLE_DIR)/Makefile

# 默认目标：构建（需指定 CASE）
all: build

build:
ifeq ($(CASE),)
	@echo "[ERROR] 未指定 CASE。可用示例：" && echo "$(EXAMPLES)" && echo "用法: make CASE=<示例名>" && exit 2
else ifeq (,$(wildcard $(EXAMPLE_MK)))
	@echo "[ERROR] 未找到示例 '$(CASE)'（应存在 $(EXAMPLE_MK)）。可用示例：" && echo "$(EXAMPLES)" && exit 2
else
	$(MAKE) -C $(EXAMPLE_DIR) all
endif

dbg:
ifeq ($(CASE),)
	@echo "[ERROR] 未指定 CASE。可用示例：" && echo "$(EXAMPLES)" && echo "用法: make dbg CASE=<示例名>" && exit 2
else ifeq (,$(wildcard $(EXAMPLE_MK)))
	@echo "[ERROR] 未找到示例 '$(CASE)'（应存在 $(EXAMPLE_MK)）。可用示例：" && echo "$(EXAMPLES)" && exit 2
else
	$(MAKE) -C $(EXAMPLE_DIR) dbg
endif

clean:
ifeq ($(CASE),)
	@echo "[INFO] 未指定 CASE，若要清理全部请执行: make clean-all"
else ifeq (,$(wildcard $(EXAMPLE_MK)))
	@echo "[WARN] 未找到示例 '$(CASE)'（应存在 $(EXAMPLE_MK)）。可用示例：" && echo "$(EXAMPLES)" && exit 0
else
	$(MAKE) -C $(EXAMPLE_DIR) clean
endif

# 清理全部示例
clean-all:
ifeq ($(strip $(EXAMPLES)),)
	@echo "[INFO] 未发现示例。"
else
	@set -e; \
	for e in $(EXAMPLES); do \
	  echo "[CLEAN] $$e"; \
	  $(MAKE) -C Projects/Examples/$$e/GCC clean || true; \
	done
endif

list:
	@echo "可用 CASE 示例："
ifeq ($(strip $(EXAMPLES)),)
	@echo "  (空)"
else
	@printf "  %s\n" $(EXAMPLES)
endif

help:
	@echo "用法："
	@echo "  make list"
	@echo "  make CASE=<示例名>            # 构建"
	@echo "  make dbg CASE=<示例名>        # 调试（调用示例内的 gdb 脚本）"
	@echo "  make clean CASE=<示例名>      # 清理"
	@echo "  make clean-all               # 清理全部"

