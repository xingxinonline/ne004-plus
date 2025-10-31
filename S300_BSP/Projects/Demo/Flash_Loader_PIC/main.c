/**
 * @file main.c
 * @brief Flash Loader PIC Demo - Main Control Program
 * 
 * This program runs in SRAM1 and dynamically loads flash operation functions
 * into SRAM0 for execution. Based on OpenOCD STMQSPI architecture.
 * 
 * Architecture:
 *   SRAM1 (0x20000000): Main control program (this file)
 *   SRAM0 (0x10000000): Dynamic flash operation code (flash_ops.bin)
 *   
 * @reference https://github.com/openocd-org/openocd/tree/master/contrib/loaders/flash/stmqspi
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "board.h"
#include "uart.h"
#include "qspi_cadence.h"
#include "rcc.h"
#include "flash_ops.h"

/* Memory regions */
#define SRAM0_BASE      0x10000000UL
#define SRAM0_SIZE      (8 * 1024)
#define SRAM1_BASE      0x20000000UL
#define SRAM1_SIZE      (384 * 1024)

/* QSPI controller base - CORRECTED! */
#define QSPI_BASE       QSPI_CFG_BASE  /* 0x4000D000, NOT 0x41000000! */

/* Flash loader context */
typedef struct {
    uint32_t load_addr;             /* SRAM0 address where code is loaded */
    uint32_t code_size;             /* Size of loaded code */
    flash_read_fn_t read_fn;        /* Function pointer to flash_read_pic */
    flash_write_fn_t write_fn;      /* Function pointer to flash_write_pic */
    flash_erase_fn_t erase_fn;      /* Function pointer to flash_erase_pic */
    crc32_fn_t crc32_fn;            /* Function pointer to crc32_calc_pic */
} flash_loader_ctx_t;

/* Global context */
static flash_loader_ctx_t g_loader_ctx;

/* Binary code - included from generated flash_ops.inc */
#include "flash_ops.inc"

/**
 * @brief HardFault Handler C function
 * 
 * Called from HardFault_Handler in startup_S300.S with fault information
 * 
 * @param stack_frame Pointer to stacked registers (R0-R3, R12, LR, PC, xPSR)
 * @param cfsr Configurable Fault Status Register
 * @param hfsr HardFault Status Register
 * @param mmfar MemManage Fault Address Register
 * @param bfar BusFault Address Register
 */
void HardFault_Handler_C(uint32_t *stack_frame, uint32_t cfsr, uint32_t hfsr, 
                         uint32_t mmfar, uint32_t bfar)
{
    printf("\n\n*** HardFault Exception ***\n");
    printf("Stack Frame:\n");
    printf("  R0  = 0x%08lX\n", stack_frame[0]);
    printf("  R1  = 0x%08lX\n", stack_frame[1]);
    printf("  R2  = 0x%08lX\n", stack_frame[2]);
    printf("  R3  = 0x%08lX\n", stack_frame[3]);
    printf("  R12 = 0x%08lX\n", stack_frame[4]);
    printf("  LR  = 0x%08lX\n", stack_frame[5]);
    printf("  PC  = 0x%08lX (return address)\n", stack_frame[6]);
    printf("  xPSR= 0x%08lX\n", stack_frame[7]);
    
    printf("\nFault Status Registers:\n");
    printf("  HFSR  = 0x%08lX\n", hfsr);
    printf("  CFSR  = 0x%08lX\n", cfsr);
    
    /* Decode CFSR */
    if (cfsr & 0x0000FFFF) {  /* MemManage Fault */
        printf("  MemManage Fault Status:\n");
        if (cfsr & (1 << 7)) printf("    MMARVALID: MMFAR valid\n");
        if (cfsr & (1 << 4)) printf("    MSTKERR: Stacking error\n");
        if (cfsr & (1 << 3)) printf("    MUNSTKERR: Unstacking error\n");
        if (cfsr & (1 << 1)) printf("    DACCVIOL: Data access violation\n");
        if (cfsr & (1 << 0)) printf("    IACCVIOL: Instruction access violation\n");
        if (cfsr & (1 << 7)) printf("    MMFAR = 0x%08lX\n", mmfar);
    }
    
    if (cfsr & 0x00FF0000) {  /* BusFault */
        printf("  BusFault Status:\n");
        if (cfsr & (1 << 15)) printf("    BFARVALID: BFAR valid\n");
        if (cfsr & (1 << 12)) printf("    STKERR: Stacking error\n");
        if (cfsr & (1 << 11)) printf("    UNSTKERR: Unstacking error\n");
        if (cfsr & (1 << 10)) printf("    IMPRECISERR: Imprecise data bus error\n");
        if (cfsr & (1 << 9)) printf("    PRECISERR: Precise data bus error\n");
        if (cfsr & (1 << 8)) printf("    IBUSERR: Instruction bus error\n");
        if (cfsr & (1 << 15)) printf("    BFAR  = 0x%08lX\n", bfar);
    }
    
    if (cfsr & 0xFF000000) {  /* UsageFault */
        printf("  UsageFault Status:\n");
        if (cfsr & (1 << 25)) printf("    DIVBYZERO: Divide by zero\n");
        if (cfsr & (1 << 24)) printf("    UNALIGNED: Unaligned access\n");
        if (cfsr & (1 << 19)) printf("    NOCP: No coprocessor\n");
        if (cfsr & (1 << 18)) printf("    INVPC: Invalid PC load\n");
        if (cfsr & (1 << 17)) printf("    INVSTATE: Invalid state\n");
        if (cfsr & (1 << 16)) printf("    UNDEFINSTR: Undefined instruction\n");
    }
    
    /* Decode HFSR */
    if (hfsr & (1 << 30)) printf("  FORCED: Forced HardFault (escalated from configurable fault)\n");
    if (hfsr & (1 << 1)) printf("  VECTTBL: Vector table read fault\n");
    
    printf("\nSystem halted in HardFault handler\n");
    while(1);  /* Halt */
}

/**
 * @brief Load PIC code into SRAM0 and resolve function pointers
 * 
 * This function:
 * 1. Copies flash_ops.bin to SRAM0
 * 2. Calls get_flash_ops_table() to get function table
 * 3. Resolves function pointers with Thumb bit set
 * 
 * @return 0 on success, -1 on error
 */
int flash_loader_init(void)
{
    printf("\n=== Flash Loader PIC Initialization ===\n");
    
    /* Check code size */
    if (flash_ops_code_size > SRAM0_SIZE) {
        printf("ERROR: Code size %lu exceeds SRAM0 size %d\n", 
               (unsigned long)flash_ops_code_size, SRAM0_SIZE);
        return -1;
    }
    
    /* Copy code to SRAM0 */
    printf("Copying %lu bytes to SRAM0 @ 0x%08lX...\n", 
           (unsigned long)flash_ops_code_size, (unsigned long)SRAM0_BASE);
    
    memcpy((void *)SRAM0_BASE, flash_ops_code, flash_ops_code_size);
    
    /* Data memory barrier - ensure all writes complete */
    __DSB();
    __ISB();
    
    printf("Code loaded successfully\n");
    
    /* Get function table using get_flash_ops_table() */
    /* The first function in the binary is flash_read_pic, but we want
     * get_flash_ops_table which returns the function table address */
    
    /* For now, use fixed offsets from analysis of flash_ops.S:
     * - flash_read_pic:  offset 0x00
     * - flash_write_pic: offset varies (~0x40-0x60)
     * - flash_erase_pic: offset varies (~0xC0-0xE0)
     * - crc32_calc_pic:  offset varies (~0x100-0x120)
     * 
     * Better approach: parse the function table at runtime
     */
    
    /* Method 1: Direct offset calculation (less flexible) */
    /* We'll use the function table approach instead */
    
    /* Method 2: Call get_flash_ops_table() to get table address */
    typedef uint32_t* (*get_table_fn_t)(void);
    
    /* get_flash_ops_table is at a known offset in the binary
     * For simplicity, we'll use direct offsets for now */
    
    /* Temporary hardcoded offsets - should be replaced with proper table parsing */
    g_loader_ctx.load_addr = SRAM0_BASE;
    g_loader_ctx.code_size = flash_ops_code_size;
    
    /* Set function pointers with Thumb bit (LSB = 1 for Thumb mode) */
    /* Offsets from flash_ops.lst: read=0x0000, write=0x009A, erase=0x01F4, crc32=0x02F8 */
    g_loader_ctx.read_fn = (flash_read_fn_t)(SRAM0_BASE + 0x0000 + 1);
    g_loader_ctx.write_fn = (flash_write_fn_t)(SRAM0_BASE + 0x009A + 1);
    g_loader_ctx.erase_fn = (flash_erase_fn_t)(SRAM0_BASE + 0x01F4 + 1);
    g_loader_ctx.crc32_fn = (crc32_fn_t)(SRAM0_BASE + 0x02F8 + 1);
    
    printf("Function pointers resolved:\n");
    printf("  flash_read_pic:  0x%08lX\n", (unsigned long)g_loader_ctx.read_fn);
    printf("  flash_write_pic: 0x%08lX\n", (unsigned long)g_loader_ctx.write_fn);
    printf("  flash_erase_pic: 0x%08lX\n", (unsigned long)g_loader_ctx.erase_fn);
    printf("  crc32_calc_pic:  0x%08lX\n", (unsigned long)g_loader_ctx.crc32_fn);
    
    printf("=== Initialization Complete ===\n\n");
    
    return 0;
}

/**
 * @brief Test flash read and CRC32 calculation
 */
void flash_loader_test(void)
{
    printf("\n=== Flash Loader Test ===\n");
    
    /* CRITICAL: Ensure QSPI is in Direct/STIG mode, NOT XIP mode
     * The PIC code needs to access QSPI registers directly */
    printf("Ensuring QSPI is in Direct mode (exit XIP if needed)...\n");
    qspi_exit_xip_mode();
    
    /* Wait for QSPI to become completely idle before PIC code execution */
    printf("Waiting for QSPI idle...\n");
    volatile uint32_t *qspi_config = (volatile uint32_t *)(QSPI_BASE + 0x00);
    volatile uint32_t *qspi_cmdctrl = (volatile uint32_t *)(QSPI_BASE + 0x90);
    
    /* Clear any pending command */
    *qspi_cmdctrl = 0;
    
    /* Wait for IDLE bit (bit 31) in CONFIG register */
    uint32_t timeout = 1000000;
    while (timeout--) {
        uint32_t cfg = *qspi_config;
        if (cfg & (1u << 31)) {  /* IDLE bit set */
            break;
        }
    }
    
    if (timeout == 0) {
        printf("WARNING: QSPI did not become idle!\n");
    } else {
        printf("QSPI is idle (CONFIG=0x%08lX, CMDCTRL=0x%08lX)\n", 
               (unsigned long)*qspi_config, (unsigned long)*qspi_cmdctrl);
    }
    
    /* CRITICAL FIX: DO NOT disable QSPI controller!
     * The PIC code REQUIRES the controller to be ENABLED to execute STIG commands.
     * The previous code disabled the controller which caused STIG command timeouts (error code 2).
     * We only need to ensure:
     * 1. QSPI is NOT in XIP mode (already done by qspi_exit_xip_mode above)
     * 2. QSPI is idle (already verified above)
     * 3. QSPI is in Direct/STIG mode (default after XIP exit)
     */
    
    /* Memory barrier - ensure all previous operations complete */
    __DSB();
    __ISB();
    
    uint8_t buffer[256];
    uint32_t flash_addr = 0x0;
    uint32_t read_len = 256;
    
    printf("Reading %lu bytes from flash @ 0x%06lX...\n", 
           (unsigned long)read_len, (unsigned long)flash_addr);
    
    printf("Calling flash_read_pic @ 0x%08lX...\n", (unsigned long)g_loader_ctx.read_fn);
    printf("Parameters: addr=0x%lX, buf=0x%lX, len=%lu, qspi=0x%lX\n",
           (unsigned long)flash_addr, (unsigned long)buffer, 
           (unsigned long)read_len, (unsigned long)QSPI_BASE);
    
    /* Ensure all memory operations complete before calling PIC code */
    __DSB();
    __ISB();
    
    /* Call flash_read_pic through function pointer */
    int ret = g_loader_ctx.read_fn(flash_addr, buffer, read_len, (uint32_t)QSPI_BASE);
    
    /* Memory barriers after returning */
    __DSB();
    __ISB();
    
    printf("Returned from flash_read_pic with code %d\n", ret);
    
    if (ret != 0) {
        printf("ERROR: Flash read failed with code %d\n", ret);
        printf("Possible reasons:\n");
        printf("  1 = QSPI controller timeout (wait idle)\n");
        printf("  2 = STIG command timeout (exec_cmd)\n");
        printf("Check QSPI configuration and flash connection\n");
        return;
    }
    
    printf("Read successful!\n");
    
    /* Display first 64 bytes */
    printf("First 64 bytes:\n");
    for (int i = 0; i < 64; i++) {
        if (i % 16 == 0) printf("  %04X: ", i);
        printf("%02X ", buffer[i]);
        if (i % 16 == 15) printf("\n");
    }
    
    /* Calculate CRC32 */
    printf("\nCalculating CRC32...\n");
    uint32_t crc = g_loader_ctx.crc32_fn(buffer, read_len);
    printf("CRC32: 0x%08lX\n", (unsigned long)crc);
    
    printf("=== Test Complete ===\n\n");
}

/**
 * @brief Simple command interpreter
 */
void command_loop(void)
{
    char cmd[32];
    
    printf("\nFlash Loader PIC Demo\n");
    printf("Commands:\n");
    printf("  read <addr> <len>  - Read flash\n");
    printf("  info               - Show loader info\n");
    printf("  help               - This help\n");
    printf("\n> ");
    
    while (1) {
    /* Simple UART input handling */
    int idx = 0;
    while (1) {
        uint16_t ch = read_uart(UART_IDX3, UARTTYPE_STD_SERIAL);
        
        if (ch == '\r' || ch == '\n') {
            cmd[idx] = '\0';
            printf("\n");
            break;
        }
        
        if (idx < sizeof(cmd) - 1) {
            cmd[idx++] = (char)ch;
            write_uart(UART_IDX3, UARTTYPE_STD_SERIAL, ch);
        }
    }        if (strlen(cmd) == 0) {
            printf("> ");
            continue;
        }
        
        /* Parse command */
        if (strcmp(cmd, "help") == 0) {
            printf("Commands:\n");
            printf("  read <addr> <len>  - Read flash\n");
            printf("  info               - Show loader info\n");
            printf("  help               - This help\n");
        }
        else if (strcmp(cmd, "info") == 0) {
            printf("Flash Loader Info:\n");
            printf("  Load address: 0x%08lX\n", (unsigned long)g_loader_ctx.load_addr);
            printf("  Code size:    %lu bytes\n", (unsigned long)g_loader_ctx.code_size);
            printf("  SRAM0 free:   %lu bytes\n", 
                   (unsigned long)(SRAM0_SIZE - g_loader_ctx.code_size));
        }
        else {
            printf("Unknown command: %s\n", cmd);
        }
        
        printf("> ");
    }
}

int main(void)
{
    /* Initialize hardware */
    board_init();
    board_debug_uart_init();  /* UART3 for debug/printf */
    
    printf("\n\n");
    printf("========================================\n");
    printf("  Flash Loader PIC Demo\n");
    printf("  Based on OpenOCD STMQSPI Architecture\n");
    printf("========================================\n");
    
    printf("\nMemory Layout:\n");
    printf("  SRAM0: 0x%08lX - 0x%08lX (%d KB)\n", 
           (unsigned long)SRAM0_BASE, 
           (unsigned long)(SRAM0_BASE + SRAM0_SIZE - 1),
           SRAM0_SIZE / 1024);
    printf("  SRAM1: 0x%08lX - 0x%08lX (%d KB)\n",
           (unsigned long)SRAM1_BASE,
           (unsigned long)(SRAM1_BASE + SRAM1_SIZE - 1),
           SRAM1_SIZE / 1024);
    
    /* Initialize QSPI */
    printf("\nInitializing QSPI...\n");
    qspi_cadence_init(100000000, 25000000);
    
    /* Check QSPI status */
    printf("QSPI Status after init:\n");
    printf("  CONFIG   = 0x%08lX\n", *(volatile uint32_t*)(QSPI_BASE + 0x00));
    printf("  CMDCTRL  = 0x%08lX\n", *(volatile uint32_t*)(QSPI_BASE + 0x90));
    
    uint8_t id[3];
    int id_ret = qspi_read_id(id, 3);
    printf("qspi_read_id returned: %d\n", id_ret);
    if (id_ret == 0) {
        printf("Flash ID: %02X %02X %02X\n", id[0], id[1], id[2]);
    } else {
        printf("Flash ID read failed!\n");
    }
    
    /* Load flash operations code */
    if (flash_loader_init() != 0) {
        printf("FATAL: Flash loader initialization failed\n");
        while (1);
    }
    
    /* Run test */
    flash_loader_test();
    
    /* Enter command loop */
    command_loop();
    
    return 0;
}
