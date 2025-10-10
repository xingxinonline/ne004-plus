set pagination off
set confirm off
set remotetimeout 20

# Make sure we operate on the Cortex-M4 (avoid touching the M0 AON target)
monitor targets ne005.m4

# Halt and soft-reset only the selected core, then load the image into Flash
monitor halt
# monitor soft_reset_halt
# Wait until the target is really halted to avoid register write errors
monitor wait_halt 2000
load

# SBL: Vector table is linked at 0x08010000 (Flash). Set SP and PC from vector.
set $sp = *(unsigned int*)0x08010000
set $pc = *(unsigned int*)0x08010004

# Set VTOR to 0x08010000 for SBL (Flash execution)
set {unsigned int}0xE000ED08 = 0x08010000

echo \n>>> Starting SBL program...\n
continue