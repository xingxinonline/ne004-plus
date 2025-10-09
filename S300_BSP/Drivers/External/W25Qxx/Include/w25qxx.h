#ifndef S300_BSP_W25QXX_H
#define S300_BSP_W25QXX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "qspi_cadence.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W25QXX_PAGE_SIZE             (256u)
#define W25QXX_SUBSECTOR_SIZE        (4096u)
#define W25QXX_BLOCK_SIZE_64K        (64u * 1024u)

typedef enum
{
    W25QXX_FLASH_UNKNOWN = 0,
    W25QXX_FLASH_W25Q,
    W25QXX_FLASH_N25Q,
    W25QXX_FLASH_MX25L
} w25qxx_flash_type_t;

typedef enum
{
    W25QXX_READMODE_FAST_1_1_1 = 0,
    W25QXX_READMODE_FAST_1_1_4,
    W25QXX_READMODE_FAST_1_4_4
} w25qxx_read_mode_t;

typedef struct
{
    uint8_t manuf_id;
    uint8_t memory_type;
    uint8_t capacity;
    uint32_t size_bytes;
    uint32_t page_size;
    uint32_t subsector_size;
    uint32_t block_size;
    w25qxx_flash_type_t type;
    const char *type_name;
    bool quad_enabled;
    bool addr4b;
    uint8_t unique_id[8];
} w25qxx_info_t;

typedef struct
{
    uintptr_t reg_base;
    uintptr_t ahb_base;
    uint32_t ref_clk_hz;
    uint32_t trigger_address;
    uint32_t sram_partition;
} w25qxx_bus_config_t;

typedef struct
{
    cqspi_dev_t controller;
    w25qxx_info_t info;
    uint32_t cached_rd_instr;
    uint32_t cached_mode_bits;
    bool cached_read_config_valid;
    bool direct_mode_active;
    w25qxx_read_mode_t current_read_mode;
} w25qxx_device_t;

typedef struct
{
    uint32_t config;
    uint32_t rd_instr;
    uint32_t mode_bit;
} w25qxx_xip_state_t;

int w25qxx_init(w25qxx_device_t *dev,
                const w25qxx_bus_config_t *bus_cfg,
                uint32_t default_sclk_hz,
                bool want_quad,
                bool want_4byte_addr,
                w25qxx_info_t *out_info);

const w25qxx_info_t *w25qxx_get_info(const w25qxx_device_t *dev);
cqspi_dev_t *w25qxx_get_controller(w25qxx_device_t *dev);

int w25qxx_configure_clock(w25qxx_device_t *dev, uint32_t target_hz);
int w25qxx_select_read_mode(w25qxx_device_t *dev, w25qxx_read_mode_t mode);

int w25qxx_read_jedec_id(w25qxx_device_t *dev, uint8_t id[3]);
int w25qxx_read_status1(w25qxx_device_t *dev, uint8_t *status);
int w25qxx_read_status2(w25qxx_device_t *dev, uint8_t *status);
int w25qxx_write_status2(w25qxx_device_t *dev, uint8_t status);
int w25qxx_write_enable(w25qxx_device_t *dev);
int w25qxx_wait_busy_clear(w25qxx_device_t *dev, uint32_t timeout_ms);
int w25qxx_disable_block_protect(w25qxx_device_t *dev);
int w25qxx_set_address_mode(w25qxx_device_t *dev, bool addr4b);
int w25qxx_enable_quad_mode(w25qxx_device_t *dev, bool enable);

int w25qxx_direct_mode_begin(w25qxx_device_t *dev);
int w25qxx_direct_mode_end(w25qxx_device_t *dev);
volatile uint8_t *w25qxx_direct_base(const w25qxx_device_t *dev);

int w25qxx_direct_write(w25qxx_device_t *dev, uint32_t address, const uint8_t *data, size_t length);
int w25qxx_direct_read(w25qxx_device_t *dev, uint32_t address, uint8_t *data, size_t length);

int w25qxx_erase_subsector(w25qxx_device_t *dev, uint32_t address);
int w25qxx_erase_block64(w25qxx_device_t *dev, uint32_t address);

int w25qxx_enter_xip_144(w25qxx_device_t *dev, w25qxx_xip_state_t *state);
int w25qxx_exit_xip(w25qxx_device_t *dev, const w25qxx_xip_state_t *state, uint32_t flush_address);
int w25qxx_issue_legacy_read(w25qxx_device_t *dev, uint32_t address, uint8_t *byte_out);

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_W25QXX_H */
