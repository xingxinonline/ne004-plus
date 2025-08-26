# S300 Minimal CMSIS BSP

## Kept

- CMSIS Device: `CMSIS/Device/PiMCHIP/S300/Include` and `.../Source` (SystemInit, SysTick, startup, vector)
- Linker: `ld/sram.ld` (vector and code in SRAM1, stack at top of SRAM1)
- Minimal app: `Projects/Minimal` (register-level UART print + SysTick heartbeat)

## Assumptions

- Default 24MHz clock (no RCC reconfiguration)
- UART index 3 (115200 baud). Adjust `UART_DEBUG_IDX` in `Projects/Minimal/Src/main.c` if required.

## Build

- From `S300_BSP/`: `make` to build the minimal example (`build/s300_minimal.elf`/`.bin`).

