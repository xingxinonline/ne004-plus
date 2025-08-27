#ifndef S300_BSP_I2S_H
#define S300_BSP_I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "s300.h"
#include "i2s_s300.h"

typedef enum { I2S_IDX0 = 0, I2S_IDX1 = 1 } i2s_idx_t;

typedef enum { I2S_WORD_12 = 1, I2S_WORD_16 = 2, I2S_WORD_20 = 3, I2S_WORD_24 = 4, I2S_WORD_32 = 5 } i2s_word_t;

/* Basic controls */
void i2s_clock_enable(i2s_idx_t i, bool en);
void i2s_set_sclk(i2s_idx_t i, uint8_t sclk_cycles, uint8_t sclk_gate);
void i2s_basic_init(i2s_idx_t i, uint32_t mclk_hz, i2s_word_t word);
void i2s_enable(i2s_idx_t i, bool en);
void i2s_tx_enable(i2s_idx_t i, bool en);
void i2s_rx_enable(i2s_idx_t i, bool en);
void i2s_fifo_flush(i2s_idx_t i, bool rx, bool tx);
void i2s_set_wordlen(i2s_idx_t i, i2s_word_t word);
void i2s_set_dma(i2s_idx_t i, bool tx_en, bool rx_en);
uint32_t i2s_sr(i2s_idx_t i);
void i2s_write_stereo(i2s_idx_t i, uint16_t l, uint16_t r);
uint32_t i2s_read_stereo(i2s_idx_t i);

/* RCC helpers (from BSP RCC) */
void rcc_set_i2s_clock(uint8_t div);

/* Legacy inline aliases for smooth porting (optional) */
static inline void set_i2s_clock_enable(int i, int en){ i2s_clock_enable((i2s_idx_t)i, en!=0); }
static inline void set_i2s_sclk_cycles(int i, uint32_t pro, uint32_t cyc, uint32_t gate){ (void)pro; i2s_set_sclk((i2s_idx_t)i, (uint8_t)cyc, (uint8_t)gate); }
static inline void set_i2s_Fifo_flushes(int i, uint32_t pro, int en){ i2s_fifo_flush((i2s_idx_t)i, (pro&0x0)?false:true, (pro&0x4)?true:false); (void)en; }
static inline void set_i2s_resolution_bit(int i, uint32_t pro, uint32_t word, int isTDM){ (void)pro; (void)isTDM; i2s_set_wordlen((i2s_idx_t)i, (i2s_word_t)word); }
static inline void set_i2s_interrupt_mask(int i, uint32_t m, int en){ (void)i; (void)m; (void)en; /* TODO: mask API if needed */ }
static inline void set_i2s_en(int i, int en){ i2s_enable((i2s_idx_t)i, en!=0); }
static inline void set_i2s_stereo_data(int i, uint16_t l, uint16_t r){ i2s_write_stereo((i2s_idx_t)i, l, r); }
static inline uint32_t get_i2s_stereo_data(int i){ return i2s_read_stereo((i2s_idx_t)i); }
static inline void set_i2s_transmitter_block_en(int i, int en){ i2s_tx_enable((i2s_idx_t)i, en!=0); }
static inline void set_i2s_recvier_block_en(int i, int en){ i2s_rx_enable((i2s_idx_t)i, en!=0); }
static inline int get_i2s_transmitter_fifo_empty_status(int i){ return (int)i2s_sr((i2s_idx_t)i); }
static inline void set_i2s_basic_config(int i, uint32_t mclk, uint32_t word){ i2s_basic_init((i2s_idx_t)i, mclk, (i2s_word_t)word); }

#ifdef __cplusplus
}
#endif

#endif /* S300_BSP_I2S_H */
