# S300 BSP Layout

- CMSIS/
  - Core/Include: CMSIS 5.9.0 core headers
  - Device/PiMCHIP/S300/Include: SoC headers (device.h, s300.h)
  - Device/PiMCHIP/S300/Source: system initialization and startup (SystemInit, SysTick, startup)
- Drivers/
  - SoC/Include: RCC & IOMUX helpers used by board configs
  - BSP/Include: BSP facades (BSP_Clock_Init, BSP_UART_Debug_Init, SysTick APIs)
  - UART: Polling UART driver
  - GPIO: Minimal GPIO (placeholder, adjust to real spec)
- Boards/
  - S300_EVB: Board-level clock & pinmux, debug UART selection via BOARD_UART_DEBUG_ID/BAUD
- Projects/
  - Examples/UART_HelloWorld: GCC Makefile project and gdbinit
- ld/
  - sram.ld: SRAM boot linker script

Build:
 Top-level `make` builds the UART_HelloWorld example.
