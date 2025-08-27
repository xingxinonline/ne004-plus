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
    /* Bits mapping per legacy: [4:3]=SCLK cycles select (0:16, 2:24, 4:32), [2:0]=gating cycles */
    I->CCR &= ~(0x18u | 0x7u);
    I->CCR |= ((uint32_t)sclk_cycles & 0x18u) | ((uint32_t)sclk_gate & 0x7u);
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
    /* Enable audio clock domain and release reset */
    rcc_set_audio_clock((uint8_t)i, true);
    rcc_set_audio_reset((uint8_t)i, true);
    i2s_clock_enable(i, true);

    /* Select SCLK cycles per word length, compute fsck = 15k * 2 * wordlen */
    uint32_t fsck;
    if (word == I2S_WORD_16 || word == I2S_WORD_12) { i2s_set_sclk(i, 0, 0); fsck = 15000u * 2u * 16u; }
    else if (word == I2S_WORD_20 || word == I2S_WORD_24) { i2s_set_sclk(i, 8, 0); fsck = 15000u * 2u * 24u; }
    else { i2s_set_sclk(i, 16, 0); fsck = 15000u * 2u * 32u; }
    /* div reg: CM4_I2S_CLK_DIV = div, actual division is 2*(div+1). Legacy: clk_div = mclk/fsck; set (clk_div/2 -1) */
    uint32_t clk_div = (mclk_hz / fsck);
    rcc_set_i2s_clock((uint8_t)((clk_div / 2u) - 1u));
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

void i2s_set_interrupt_mask(i2s_idx_t i, uint32_t mask, bool en)
{
    S300_I2S_TypeDef *I = i2s_dev(i);
    if (en) I->IMR0 |= mask; else I->IMR0 &= ~mask;
}

/* DMA helper following legacy usage: RX: P2M from I2S_RXDMA; TX: M2P to I2S_TXDMA */
void i2s_dma_mode(i2s_idx_t i, uint8_t rec_chn, uint8_t play_chn, uint8_t *rec_buffer, uint8_t *play_buffer, uint32_t length)
{
    (void)i; /* On this SoC, handshake IDs below use I2S1 in examples; adapt if needed by index */
    /* Record: peripheral (I2S RXDMA) -> memory */
    set_dma_std(EM_DMA0, rec_chn, (uint32_t)&i2s_dev(i)->RXDMA, (uint32_t)rec_buffer, length, EM_TR_WIDTH_16_BIT);
    set_dma_std_increment(EM_DMA0, rec_chn, EM_ADDRESS_UNCHANGE, EM_ADDRESS_INC);
    set_dma_std_transfer_type(EM_DMA0, rec_chn, EM_TR_TYPE_P2M_FD);
    set_dma_handshaking(EM_DMA0, rec_chn, (i==I2S_IDX0)?EM_HAND_I2S0_RX:EM_HAND_I2S1_RX, EM_HAND_NULL);
    set_dma_interrupt(EM_DMA0, rec_chn, EM_DMA_INT_TFR, 1);

    /* Play: memory -> peripheral (I2S TXDMA) */
    set_dma_std(EM_DMA0, play_chn, (uint32_t)play_buffer, (uint32_t)&i2s_dev(i)->TXDMA, length, EM_TR_WIDTH_16_BIT);
    set_dma_std_increment(EM_DMA0, play_chn, EM_ADDRESS_INC, EM_ADDRESS_UNCHANGE);
    set_dma_std_transfer_type(EM_DMA0, play_chn, EM_TR_TYPE_M2P_FD);
    set_dma_handshaking(EM_DMA0, play_chn, EM_HAND_NULL, (i==I2S_IDX0)?EM_HAND_I2S0_TX:EM_HAND_I2S1_TX);
    set_dma_interrupt(EM_DMA0, play_chn, EM_DMA_INT_TFR, 1);

    /* Enable I2S DMA mask bits like legacy */
    i2s_set_dma(i, true, true);

    set_dma_start(EM_DMA0, rec_chn);
    set_dma_start(EM_DMA0, play_chn);
}
