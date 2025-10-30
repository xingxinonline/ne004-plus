# OpenOCD Flash Ops Simulation Demo

This demo simulates typical OpenOCD NOR flash operations on a W25Qxx device via QSPI (Cadence):

- Probe (JEDEC ID, SR1-3)
- Unlock protections
- Region erase (4KB sectors)
- Page program (256B) across page boundaries
- Verify (byte-by-byte + CRC32)
- Timing using 1us SysTick

It avoids XIP and uses STIG read/program/erase flows similar to OpenOCD drivers.

## Build

This demo is wired into `S300_BSP/Projects/CMakeLists.txt`.

From the workspace root:

```bash
cmake -S S300_BSP -B S300_BSP/build
cmake --build S300_BSP/build --target s300_openocd_flash_sim -j
```

Artifacts are under `S300_BSP/build/Projects/Demo/OpenOCD_Flash_Sim/`:

- `s300_openocd_flash_sim` (ELF)
- `s300_openocd_flash_sim.bin`
- `s300_openocd_flash_sim.hex`
- `s300_openocd_flash_sim.dis`

A convenience debug target is also available:

```bash
cmake --build S300_BSP/build --target dbg_openocd_flash_sim
```

This opens `arm-none-eabi-gdb` with `gdbinit.sram.gdb` to load into SRAM and run.

## Runtime

On boot, UART prints a sequence like:

```text
[SIM] OpenOCD-like Flash Ops demo start. AHB=... QSPI=...
[SIM] JEDEC: manuf=0xEF type=0x40 cap=0x18 size=...
[SIM] Status(before): SR1=... SR2=... SR3=...
[SIM] Erase 4K @0x00100000... OK (... us)
...
[SIM] Erased N sectors (4KB each)
[SIM] Program  256B @0x00100000... OK (... us)
...
[SIM] Verify OK (CRC=...)
[SIM] Status(after): ...
[SIM] Demo DONE.
```

## Parameters

- Base address: `0x00100000` (chosen to avoid boot areas)
- Length: `8KB` (adjustable)
- Image pattern: simple deterministic function of index and base

To adjust, edit `Src/main.c` constants `base` and `length`.

## UART CLI

Type `help` on the UART console to see commands:

- `cfg base 0xADDR len BYTES` — set base and length for runs
- `cfg chunk KB` — program/verify chunk size (default 4KB)
- `cfg verbose 0|1` — per-page program prints
- `xip set dummy N mode 0xMM`, `xip enter`, `xip exit`
- `run` — generate small image and run erase+program+verify (mixed 64K/32K/4K)
- `erase_plan 0xADDR SIZE` — preview erase plan (64K → 32K → 4K) and addresses
- `erase_region 0xADDR SIZE` — perform mixed erase, print timing and stats
- `write_image 0xADDR SIZE` — after `READY`, send SIZE raw bytes; device programs chunked
- `verify_image 0xADDR SIZE` — compute CRC32 over flash region and print speed
- `flash_image 0xADDR SIZE` — one-shot script: erase → receive → program → verify with CRC compare and a consolidated report
- `run_1mb | run1m` — synthetic 1MiB test: mixed erase + generated program + verify

### Example host flow (binary streaming)

On `flash_image`, the device prints `READY` before expecting raw bytes. Send the bytes immediately over the same serial port.

Notes:

- `flash_image` computes CRC on-the-fly during reception and compares with CRC computed from flash after programming.
- `erase_region`/`run` print an erase plan preview highlighting 64K, 32K, and 4K coverage.

