# OpenOCD Flash SRAM0 Execution Demo

This demo mirrors the OpenOCD `stmqspi` async flashing flow directly on the S300:

- **Host supervisor** runs entirely in SRAM1 (384KB) and mimics OpenOCD logic.
- **Loader core** is linked into SRAM0 (8KB) and executes flash READ/WRITE primitives while sharing a ring buffer + mailbox, just like the OpenOCD async algorithms.
- A FreeRTOS scheduler time-slices both sides so the FIFO handshake is observable on a single-core Cortex-M4.

## Features

- Streams arbitrary byte ranges into/out of an external W25Qxx NOR using an 8-byte STIG loop (identical to the upstream loader strategy).
- Bidirectional FIFO (1KB) located in SRAM0 with write/read pointers exposed for instrumentation.
- Loader task stack + code + scratch buffers live in dedicated sections (`.loader_*`) that map to SRAM0.
- Host task feeds data from SRAM1 buffers, reports throughput, and validates CRC/byte-for-byte equivalence after each run.

## Build

From the repo root:

```bash
cmake -S S300_BSP -B S300_BSP/build
cmake --build S300_BSP/build --target s300_openocd_flash_sram0exec -j
```

Artifacts appear under `S300_BSP/build/Projects/Demo/OpenOCD_Flash_SRAM0Exec/` (ELF/bin/hex/disasm). Use `dbg_openocd_flash_sram0exec` to load into SRAM quickly via OpenOCD + GDB.

## Runtime

Upon reset the firmware:

1. Boots clocks/UART, brings up the Cadence QSPI block, probes the NOR (via the regular driver in SRAM1).
2. Starts FreeRTOS with two tasks:
   - `host_master`: prepares a deterministic pattern (default 8KB at `0x80040000`), kicks erase/program/read cycles, and prints progress.
   - `loader_async`: executes from SRAM0, polling the mailbox, and performing STIG READ / PAGE PROGRAM operations while pushing/pulling FIFO bytes.
3. After programming, the host issues a readback to validate data integrity. Results similar to:

```text
[HOST] Async write 8192 B @0x80040000 -> 54.2 KB/s
[HOST] Async read  8192 B @0x80040000 -> 58.1 KB/s
[HOST] CRC32 tx=0x8E2C1CF7 rx=0x8E2C1CF7
[HOST] Demo complete.
```

Adjust `test_len`, `test_base`, and FIFO size in `Src/main.c` / `Src/loader.h` to exercise different spans (e.g., Octal mode or dual-flash).

## File Layout

```
Src/
  main.c        # Host supervisor + FreeRTOS bootstrap
  loader.c      # SRAM0-resident loader logic, FIFO helpers, QSPI primitives
  loader.h      # Shared mailbox/FIFO definitions
ld/sram0_exec.ld # Memory split linker script
```

Refer to `docs/stmqspi_flash_read.md` for a deeper discussion of the algorithm. This demo keeps data/command structures identical so it can double as a playground before touching the actual OpenOCD driver.
