#include "i2s.h"
#include "rcc.h"

static inline S300_I2S_TypeDef *i2s_dev(i2s_idx_t i)
{
    return (i == I2S_IDX0) ? I2S0 : I2S1;
}

void i2s_clock_enable(i2s_idx_t i, bool en)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    if (en) I->CER |= 0x1u;
    else I->CER &= ~0x1u;
}

void i2s_set_sclk(i2s_idx_t i, uint8_t sclk_cycles, uint8_t sclk_gate)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    /* Disable clock generator to change CCR safely */
    I->CER = 0u;
    I->CCR = (I->CCR & ~(0x18u | 0x7u)) | ((uint32_t)sclk_cycles & 0x18u) | ((uint32_t)sclk_gate & 0x7u);
    I->CER = 0x1u;
}

void i2s_set_wordlen(i2s_idx_t i, i2s_word_t word)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    /* Disable enables for reconfig */
    I->RER0 &= ~1u; I->TER0 &= ~1u;
    I->RCR0 = (uint32_t)word;
    I->TCR0 = (uint32_t)word;
    I->RER0 |= 1u; I->TER0 |= 1u;
}

void i2s_basic_init(i2s_idx_t i, uint32_t mclk_hz, i2s_word_t word)
{
    (void)mclk_hz; /* We compute fs based on 15kHz in legacy function; here use RCC helper */
    /* Enable audio clock domain and release reset */
    rcc_set_audio_clock((uint8_t)i, true);
    rcc_set_audio_reset((uint8_t)i, true);
    i2s_clock_enable(i, true);
    if (word == I2S_WORD_16 || word == I2S_WORD_12)
    {
        i2s_set_sclk(i, 0 /* 16 cycles -> bits[4:3]=0 */ , 0 /* gate disable */);
        rcc_set_i2s_clock( (uint8_t)((rcc_get_clock(RCC_CLOCK_AUDIO) / (15000u*2u*16u))/2u - 1u) );
    }
    else if (word == I2S_WORD_20 || word == I2S_WORD_24)
    {
        i2s_set_sclk(i, 8 /* 24 cycles -> bits[4:3]=b10 */ , 0);
        rcc_set_i2s_clock( (uint8_t)((rcc_get_clock(RCC_CLOCK_AUDIO) / (15000u*2u*24u))/2u - 1u) );
    }
    else
    {
        i2s_set_sclk(i, 16 /* 32 cycles -> bits[4:3]=b10000? keep legacy value */ , 0);
        rcc_set_i2s_clock( (uint8_t)((rcc_get_clock(RCC_CLOCK_AUDIO) / (15000u*2u*32u))/2u - 1u) );
    }
    i2s_set_wordlen(i, word);
    i2s_tx_enable(i, true);
    i2s_rx_enable(i, true);
}

void i2s_enable(i2s_idx_t i, bool en)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    if (en)
    {
        i2s_fifo_flush(i, true, true);
        I->IER |= 0x1u;
    }
    else I->IER &= ~0x1u;
}

void i2s_tx_enable(i2s_idx_t i, bool en)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    if (en) I->ITER |= 0x1u; else I->ITER &= ~0x1u;
}

void i2s_rx_enable(i2s_idx_t i, bool en)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    if (en) I->IRER |= 0x1u; else I->IRER &= ~0x1u;
}

void i2s_fifo_flush(i2s_idx_t i, bool rx, bool tx)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    if (rx) I->RXFFR = 1u; else I->RXFFR = 0u;
    if (tx) I->TXFFR = 1u; else I->TXFFR = 0u;
}

void i2s_set_dma(i2s_idx_t i, bool tx_en, bool rx_en)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    uint32_t v = I->DMACR;
    v &= ~(0x03u << 16);
    v |= ((tx_en ? 1u:0u) << 17) | ((rx_en ? 1u:0u) << 16);
    I->DMACR = v;
}

uint32_t i2s_sr(i2s_idx_t i)
{
    return i2s_dev(i)->SR;
}

void i2s_write_stereo(i2s_idx_t i, uint16_t l, uint16_t r)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    I->LRBR0 = l; /* left tx */
    I->RRBR0 = r; /* right tx */
}

uint32_t i2s_read_stereo(i2s_idx_t i)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    uint32_t l = I->LRBR0 & 0xFFFFu;
    uint32_t r = I->RRBR0 & 0xFFFFu;
    return (l << 16) | r;
}
