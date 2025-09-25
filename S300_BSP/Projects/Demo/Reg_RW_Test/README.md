# S300 Register Read/Write Test Demo (Frontend Simulation Version)

This is a simplified register read/write test demo for the PiMCHIP S300 platform, designed for frontend simulation environments. It performs basic register read/write operations and reports results through debug memory variables instead of UART output.

## Overview

The demo performs basic register read/write operations to verify hardware functionality:

- GPIO registers (SWPORTA_DR, SWPORTA_DDR, SWPORTA_CTL)
- RCC registers (CM4_AHB_CLK_EN, CM4_APB0_CLK_EN)

## Key Differences from Standard Version

- **No UART output**: Removed all `printf` calls
- **No board initialization**: Removed `board_init()` call
- **Debug memory interface**: Uses memory-mapped debug variables for status reporting
- **Minimal dependencies**: Only essential drivers included

## Debug Memory Interface

The demo uses the following debug memory locations (based at `M4_SLV_RAM0_END - 0x3F`):

| Address Offset | Variable     | Description                                                                                  |
| -------------- | ------------ | -------------------------------------------------------------------------------------------- |
| 0x0000         | DEBUG_ARG0_c | Test result for register 0 (0xAAAAAAAA = PASS, 0xEEEEEEEE = FAIL)                            |
| 0x0004         | DEBUG_ARG1_c | Test result for register 1                                                                   |
| 0x0008         | DEBUG_ARG2_c | Test result for register 2                                                                   |
| 0x000C         | DEBUG_ARG3_c | Test result for register 3                                                                   |
| 0x0010         | DEBUG_ST_c   | Test status (0xAAAAAAAA = ALL PASS, 0xEEEEEEEE = SOME FAIL, 0xBBBBBBBB = IN PROGRESS)        |
| 0x0014         | DEBUG_ISRS_c | Interrupt counter (unused in this demo)                                                      |
| 0x0018         | DEBUG_CODE_c | Program completion status (0xAAAAAAAA = SUCCESS, 0xEEEEEEEE = FAILED, 0xCCCCCCCC = STARTING) |
| 0x001C         | DEBUG_FLOW_c | Test progress counter (1-5 indicating current test)                                          |

## Building

```bash
cd Projects/Demo/Reg_RW_Test/GCC
make
```

## Test Results

After running, check the debug memory locations:

- **DEBUG_CODE_c = 0xAAAAAAAA**: All tests passed
- **DEBUG_CODE_c = 0xEEEEEEEE**: Some tests failed
- **DEBUG_ARG0_c - DEBUG_ARG3_c**: Individual test results
- **DEBUG_ST_c**: Overall test status

## Architecture

This demo maintains the same test structure as the standard version but replaces UART-based reporting with memory-based debug variables, making it suitable for frontend simulation environments where UART output may not be available or desired.