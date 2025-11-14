#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "loader.h"
#include "board.h"
#include "rcc.h"
#include "qspi_cadence.h"
#include "w25qxx.h"

#define HOST_TASK_STACK       1024u
#define HOST_TASK_PRIORITY    (configMAX_PRIORITIES - 2)
#define LOADER_TASK_PRIORITY  (configMAX_PRIORITIES - 3)
#define LOADER_STACK_WORDS  512u

#define TEST_LENGTH (8u * 1024u)
#define TEST_BASE   0x80040000u

static StaticTask_t g_loader_tcb;
__attribute__((section(".loader.stack"))) static StackType_t g_loader_stack[LOADER_STACK_WORDS];
static TaskHandle_t g_loader_handle;
static TaskHandle_t g_host_handle;

static w25qxx_info_t g_flash_info;
static uint8_t g_tx_buffer[TEST_LENGTH];
static uint8_t g_rx_buffer[TEST_LENGTH];

static void host_task(void *param);
static void loader_mailbox_host_init(const w25qxx_info_t *info);
static void loader_prepare_command(loader_mailbox_t *mb, uint32_t cmd, uint32_t addr, uint32_t len);
static int flash_async_write(uint32_t addr, const uint8_t *data, uint32_t len);
static int flash_async_read(uint32_t addr, uint8_t *data, uint32_t len);
static int erase_region_4k(uint32_t addr, uint32_t len);
static uint32_t crc32_le(const uint8_t *data, uint32_t len);
static uint32_t fifo_level(const loader_mailbox_t *mb);
static uint32_t fifo_space(const loader_mailbox_t *mb);
static void fifo_push(loader_mailbox_t *mb, const uint8_t *src, uint32_t len);
static void fifo_pop(loader_mailbox_t *mb, uint8_t *dst, uint32_t len);
static void wait_for_idle(loader_mailbox_t *mb);

void vAssertCalled(const char *file, int line)
{
    taskDISABLE_INTERRUPTS();
    printf("Assert: %s:%d\r\n", file, line);
    while (1) {
        __asm volatile ("nop");
    }
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pxTask;
    printf("[HOST] Stack overflow: %s\r\n", pcTaskName ? pcTaskName : "?");
    vAssertCalled(__FILE__, __LINE__);
}

void vApplicationMallocFailedHook(void)
{
    printf("[HOST] Malloc failed\r\n");
    vAssertCalled(__FILE__, __LINE__);
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    static StaticTask_t idle_tcb;
    static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
    *ppxIdleTaskTCBBuffer = &idle_tcb;
    *ppxIdleTaskStackBuffer = idle_stack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t timer_tcb;
    static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer = &timer_tcb;
    *ppxTimerTaskStackBuffer = timer_stack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

int main(void)
{
    board_init();

    uint32_t ahb_hz = rcc_get_clock(RCC_CLOCK_AHB);
    uint32_t sclk_hz = ahb_hz / 2u;

    printf("\r\n[HOST] OpenOCD SRAM0Exec demo start. AHB=%lu Hz QSPI=%lu Hz\r\n",
           (unsigned long)ahb_hz, (unsigned long)sclk_hz);

    qspi_set_verbose(false);
    qspi_cadence_init(ahb_hz, sclk_hz);

    bool force_qe = true;
    bool force_4b = false;
    if (w25qxx_init(&g_flash_info, force_qe, force_4b) != 0) {
        printf("[HOST] w25qxx_init failed\r\n");
        for (;;) {}
    }
    (void)qspi_unlock_all();

    printf("[HOST] JEDEC %02X %02X %02X size=%lu addr4b=%u\r\n",
           g_flash_info.manuf_id,
           g_flash_info.memory_type,
           g_flash_info.capacity,
           (unsigned long)g_flash_info.size_bytes,
           g_flash_info.addr4b ? 1u : 0u);

    loader_mailbox_host_init(&g_flash_info);

    for (uint32_t i = 0; i < TEST_LENGTH; ++i) {
        g_tx_buffer[i] = (uint8_t)(i ^ 0x5Au) + (uint8_t)((TEST_BASE >> 8) & 0xFFu);
    }

    g_loader_handle = xTaskCreateStatic(loader_task,
                                        "loader",
                                        LOADER_STACK_WORDS,
                                        NULL,
                                        LOADER_TASK_PRIORITY,
                                        g_loader_stack,
                                        &g_loader_tcb);
    configASSERT(g_loader_handle != NULL);

    BaseType_t ok = xTaskCreate(host_task, "host", HOST_TASK_STACK, NULL, HOST_TASK_PRIORITY, &g_host_handle);
    configASSERT(ok == pdPASS);

    vTaskStartScheduler();
    for (;;) {}
}

static void host_task(void *param)
{
    (void)param;
    const uint32_t base = TEST_BASE;
    const uint32_t len = TEST_LENGTH;

    printf("[HOST] Target region 0x%08lX len=%lu\r\n",
           (unsigned long)base, (unsigned long)len);

    for (;;) {
        memset(g_rx_buffer, 0, sizeof(g_rx_buffer));

        if (erase_region_4k(base, len) != 0) {
            printf("[HOST] Erase failed\r\n");
            break;
        }

        uint32_t t0 = xTaskGetTickCount();
        int rc = flash_async_write(base, g_tx_buffer, len);
        uint32_t t1 = xTaskGetTickCount();
        if (rc != 0) {
            printf("[HOST] async write failed rc=%d status=%lu err=%lu\r\n",
                   rc,
                   (unsigned long)g_loader_mailbox.status,
                   (unsigned long)g_loader_mailbox.error_code);
            break;
        }
        float elapsed_ms = (float)(t1 - t0);
        float speed_kb = ((float)len / 1024.0f) / (elapsed_ms / 1000.0f);
        printf("[HOST] Write %lu bytes -> %.2f KB/s\r\n", (unsigned long)len, speed_kb);

        t0 = xTaskGetTickCount();
        rc = flash_async_read(base, g_rx_buffer, len);
        t1 = xTaskGetTickCount();
        if (rc != 0) {
            printf("[HOST] async read failed rc=%d status=%lu err=%lu\r\n",
                   rc,
                   (unsigned long)g_loader_mailbox.status,
                   (unsigned long)g_loader_mailbox.error_code);
            break;
        }
        elapsed_ms = (float)(t1 - t0);
        speed_kb = ((float)len / 1024.0f) / (elapsed_ms / 1000.0f);
        printf("[HOST] Read  %lu bytes -> %.2f KB/s\r\n", (unsigned long)len, speed_kb);

        uint32_t crc_tx = crc32_le(g_tx_buffer, len);
        uint32_t crc_rx = crc32_le(g_rx_buffer, len);
        printf("[HOST] CRC32 tx=0x%08lX rx=0x%08lX\r\n",
               (unsigned long)crc_tx,
               (unsigned long)crc_rx);
        if (crc_tx != crc_rx || memcmp(g_tx_buffer, g_rx_buffer, len) != 0) {
            printf("[HOST] Data mismatch!\r\n");
            break;
        }

        printf("[HOST] Demo cycle OK. Sleeping...\r\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    printf("[HOST] Stopping test loop.\r\n");
    vTaskSuspend(NULL);
}

static void loader_mailbox_host_init(const w25qxx_info_t *info)
{
    taskENTER_CRITICAL();
    memset((void *)&g_loader_mailbox, 0, sizeof(g_loader_mailbox));
    g_loader_mailbox.cmd = LOADER_CMD_NONE;
    g_loader_mailbox.status = LOADER_STATUS_IDLE;
    g_loader_mailbox.fifo_size = LOADER_FIFO_SIZE;
    g_loader_mailbox.addr_bytes = (info && info->addr4b) ? 4u : 3u;
    g_loader_mailbox.page_size = (info && info->page_size) ? info->page_size : 256u;
    taskEXIT_CRITICAL();
}

static void loader_prepare_command(loader_mailbox_t *mb, uint32_t cmd, uint32_t addr, uint32_t len)
{
    wait_for_idle(mb);
    taskENTER_CRITICAL();
    mb->fifo_wp = 0u;
    mb->fifo_rp = 0u;
    mb->progress = 0u;
    mb->error_code = 0u;
    mb->flash_addr = addr;
    mb->length = len;
    mb->cmd = cmd;
    mb->status = LOADER_STATUS_BUSY;
    taskEXIT_CRITICAL();
}

static int flash_async_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    loader_mailbox_t *mb = &g_loader_mailbox;
    loader_prepare_command(mb, LOADER_CMD_WRITE, addr, len);
    uint32_t produced = 0u;
    while (mb->status == LOADER_STATUS_BUSY) {
        if (produced < len) {
            uint32_t space = fifo_space(mb);
            if (space != 0u) {
                uint32_t chunk = len - produced;
                if (chunk > space) chunk = space;
                fifo_push(mb, data + produced, chunk);
                produced += chunk;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    return (mb->status == LOADER_STATUS_DONE) ? 0 : -1;
}

static int flash_async_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    loader_mailbox_t *mb = &g_loader_mailbox;
    loader_prepare_command(mb, LOADER_CMD_READ, addr, len);
    uint32_t consumed = 0u;
    while ((mb->status == LOADER_STATUS_BUSY) || (fifo_level(mb) > 0u)) {
        uint32_t level = fifo_level(mb);
        if (level != 0u) {
            uint32_t chunk = len - consumed;
            if (chunk > level) chunk = level;
            fifo_pop(mb, data + consumed, chunk);
            consumed += chunk;
        } else {
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    }
    return (mb->status == LOADER_STATUS_DONE) ? 0 : -1;
}

static int erase_region_4k(uint32_t addr, uint32_t len)
{
    const uint32_t sector = 0x1000u;
    if (len == 0u) return 0;
    uint32_t start = addr & ~(sector - 1u);
    uint32_t end = (addr + len + (sector - 1u)) & ~(sector - 1u);
    for (uint32_t cur = start; cur < end; cur += sector) {
        int rc = w25qxx_erase_4k(cur);
        if (rc != 0) {
            printf("[HOST] Erase 4K @0x%08lX failed rc=%d\r\n", (unsigned long)cur, rc);
            return rc;
        }
    }
    return 0;
}

static uint32_t crc32_le(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; ++i) {
        crc ^= data[i];
        for (uint32_t b = 0; b < 8u; ++b) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return crc ^ 0xFFFFFFFFu;
}

static uint32_t fifo_level(const loader_mailbox_t *mb)
{
    return mb->fifo_wp - mb->fifo_rp;
}

static uint32_t fifo_space(const loader_mailbox_t *mb)
{
    return mb->fifo_size - fifo_level(mb);
}

static void fifo_push(loader_mailbox_t *mb, const uint8_t *src, uint32_t len)
{
    uint32_t wp = mb->fifo_wp;
    for (uint32_t i = 0; i < len; ++i) {
        mb->fifo[wp & LOADER_FIFO_MASK] = src[i];
        ++wp;
    }
    mb->fifo_wp = wp;
}

static void fifo_pop(loader_mailbox_t *mb, uint8_t *dst, uint32_t len)
{
    uint32_t rp = mb->fifo_rp;
    for (uint32_t i = 0; i < len; ++i) {
        dst[i] = mb->fifo[rp & LOADER_FIFO_MASK];
        ++rp;
    }
    mb->fifo_rp = rp;
}

static void wait_for_idle(loader_mailbox_t *mb)
{
    while (mb->cmd != LOADER_CMD_NONE || mb->status == LOADER_STATUS_BUSY) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
