# QSPI W25Q128 Test Demo

This demo exercises the Cadence QSPI controller together with the Winbond W25Qxx driver ported from the Linux cadence-quadspi flow. It initializes the controller, reads identification data, dumps SFDP information, and performs basic program/erase tests in both single-lane and quad-lane modes.

## Requirements

- PiMCHIP S300 platform with the external Winbond W25Q128 (or compatible W25Qxx) device connected to the Cadence QSPI controller.
- ARM GNU toolchain (`arm-none-eabi-gcc`).
- Board UART wired to the host to observe log output.

## Building

```bash
cd Projects/Demo/QSPI_W25Q128_Test/GCC
make
```

The build produces `build/qspi_w25q128_test.elf` and `build/qspi_w25q128_test.bin`.

## What the demo does

1. Initializes board clocks and UART (via `board_init`).
2. Powers up the Cadence QSPI controller and creates a W25Qxx driver instance.
3. Prints JEDEC ID, unique ID, basic geometry, and the first 16 bytes of the SFDP table.
4. Runs the following functional tests on a scratch 4&nbsp;KiB sector:
   - Sector erase.
   - Page program using single-lane I/O followed by single-lane readback.
   - If quad mode is available, quad-read verification.
   - Quad page program and quad I/O read verification.

An example excerpt of the expected log:

```
QSPI W25Qxx Cadence Controller Demo
AHB clock: 192000000 Hz
=== Winbond W25Qxx Summary ===
JEDEC ID      : EF 40 18
Identified as : Winbond W25Q128
Capacity      : 16777216 bytes (16.00 MiB)
...
Functional test: PASS
QSPI tests PASSED.
```

If any stage fails, the demo prints an error message before entering the idle loop. The test uses offset `0x1000` for scratch operations; adjust the source if that sector must be preserved in your setup.
