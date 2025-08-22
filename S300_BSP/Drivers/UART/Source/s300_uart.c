#include "s300.h"
#include "s300_uart.h"
#include "systick.h"

#ifndef UART_INPUT_CLOCK_HZ
#ifdef BOARD_UART_SRC_HZ
#define UART_INPUT_CLOCK_HZ ((uint32_t)(BOARD_UART_SRC_HZ))
#else
#define UART_INPUT_CLOCK_HZ (SystemCoreClock)
#endif
#endif

static void s300_uart_set_baud(uint32_t base, uint32_t baud)
{
    /* DW-apb-uart style: baud_div = UART_CLK / (16 * baud), fractional via DLF (1/16 steps)
       If SoC differs, override UART_INPUT_CLOCK_HZ or adjust DLF handling. */
    uint32_t clk = UART_INPUT_CLOCK_HZ;
    uint32_t denom = baud * 16u;
    if (baud == 0u || clk == 0u) return;
    uint32_t div = clk / denom;
    uint32_t rem = clk % denom;
    /* fractional in 1/16 step, rounded */
    uint32_t dlf = (uint32_t)((uint64_t)rem * 16u + (denom/2u)) / denom;
    if (dlf >= 16u) { dlf = 0u; div += 1u; }
    if (div == 0u) div = 1u;
    /* Program divisor latch */
    UART_LCRn(base) |= 0x80; /* DLAB */
    UART_DLLn(base) = div & 0xFF;
    UART_DLHn(base) = (div >> 8) & 0xFF;
    UART_LCRn(base) &= ~0x80; /* clear DLAB */
    UART_DLFn(base) = dlf & 0x0F; /* assume 4-bit DLF (1/16). Adjust if wider. */
}

/* ===== Simple RX ring buffer for IRQ-driven receive (per UART idx 0..3) ===== */
#ifndef S300_UART_MAX
#define S300_UART_MAX 4
#endif
#ifndef S300_UART_RX_BUFSZ
#define S300_UART_RX_BUFSZ 128
#endif

typedef struct {
    volatile uint16_t head;
    volatile uint16_t tail;
    uint8_t buf[S300_UART_RX_BUFSZ];
} s300_ring_t;

static s300_ring_t s_rx_rings[S300_UART_MAX];

static inline uint32_t s_ring_count(const s300_ring_t *r)
{
    return (uint16_t)(r->head - r->tail);
}

static inline int s_ring_push(s300_ring_t *r, uint8_t b)
{
    uint16_t n = r->head - r->tail;
    if (n >= S300_UART_RX_BUFSZ) return 0; /* full */
    r->buf[r->head % S300_UART_RX_BUFSZ] = b;
    r->head++;
    return 1;
}

static inline int s_ring_pop(s300_ring_t *r, uint8_t *b)
{
    if (r->head == r->tail) return 0;
    *b = r->buf[r->tail % S300_UART_RX_BUFSZ];
    r->tail++;
    return 1;
}

void S300_UART_Init(uint32_t idx, uint32_t baud)
{
    uint32_t base = UARTn_BASE(idx);
    /* Disable UART interrupts */
    UART_IERn(base) = 0x0;
    /* FIFO: enable and reset */
    UART_FCRn(base) = 0x07;
    /* 8N1 */
    UART_LCRn(base) = 0x03;
    /* Modem control: default 0 (no auto RTS/CTS) */
    UART_MCRn(base) = 0x00;
    /* Baud */
    s300_uart_set_baud(base, baud);
    /* Wait TX FIFO ready */
    (void)UART_USRn(base);
}

/* Simple UART init for 24MHz, 115200, 8N1, polling
   Note: Board layer must have enabled clocks/resets and configured pinmux. */
void S300_UART_Init_115200(uint32_t idx)
{
    S300_UART_Init(idx, 115200u);
}

void S300_UART_PutCharI(uint32_t idx, char c)
{
    uint32_t base = UARTn_BASE(idx);
    uint32_t to = 1000000u;
    while (!((UART_USRn(base) & (1u << 1)) || (UART_LSRn(base) & (1u << 5))))
    {
        if (--to == 0u) break;
    }
    UART_THRn(base) = (uint32_t)c;
}

void S300_UART_PutStringI(uint32_t idx, const char *s)
{
    while (*s)
    {
        if (*s == '\n') S300_UART_PutCharI(idx, '\r');
        S300_UART_PutCharI(idx, *s++);
    }
}

/* ===== Extended API implementations ===== */

int S300_UART_Config(uint32_t idx, const S300_UartConfig *cfg)
{
    if (!cfg) return -1;
    uint32_t base = UARTn_BASE(idx);
    uint32_t lcr = 0;
    /* data bits */
    uint8_t db = (cfg->data_bits >= 5 && cfg->data_bits <= 8) ? cfg->data_bits : 8;
    lcr |= (uint32_t)((db - 5u) & 0x3u);
    /* stop bits */
    if (cfg->stop_bits >= 2) lcr |= (1u << 2);
    /* parity */
    if (cfg->parity == 1) { /* odd */
        lcr |= (1u << 3);
    } else if (cfg->parity == 2) { /* even */
        lcr |= (1u << 3); /* PEN */
        lcr |= (1u << 4); /* EPS */
    }
    UART_LCRn(base) = lcr;
    /* FIFO */
    if (cfg->fifo_enable) UART_FCRn(base) = 0x01; else UART_FCRn(base) = 0x00;
    /* baud */
    s300_uart_set_baud(base, cfg->baud ? cfg->baud : 115200u);
    return 0;
}

int S300_UART_TxReady(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    /* Prefer TFNF (USR[1]) indicates TX FIFO not full; fallback THRE in LSR[5] */
    return ((UART_USRn(base) & (1u << 1)) != 0u) || ((UART_LSRn(base) & (1u << 5)) != 0u);
}

int S300_UART_TxIdle(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    return (UART_LSRn(base) & (1u << 6)) != 0u; /* TEMT */
}

int S300_UART_RxReady(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    /* RFNE (USR[3]) or LSR.DR */
    return ((UART_USRn(base) & (1u << 3)) != 0u) || ((UART_LSRn(base) & 1u) != 0u);
}

int S300_UART_TryWrite(uint32_t idx, uint8_t byte)
{
    uint32_t base = UARTn_BASE(idx);
    if (!S300_UART_TxReady(idx)) return 0;
    UART_THRn(base) = byte;
    return 1;
}

int S300_UART_Write(uint32_t idx, const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (!buf) return -1;
    uint32_t base = UARTn_BASE(idx);
    uint32_t start = S300_SysTick_Millis();
    size_t i = 0;
    while (i < len) {
        if (S300_UART_TryWrite(idx, buf[i])) {
            i++;
            continue;
        }
        if (timeout_ms) {
            if ((uint32_t)(S300_SysTick_Millis() - start) >= timeout_ms) break;
        }
    }
    return (int)i;
}

int S300_UART_TryRead(uint32_t idx, uint8_t *byte)
{
    if (!byte) return -1;
    uint32_t base = UARTn_BASE(idx);
    if (!S300_UART_RxReady(idx)) return 0;
    *byte = (uint8_t)UART_RBRn(base);
    return 1;
}

int S300_UART_Read(uint32_t idx, uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    if (!buf) return -1;
    uint32_t start = S300_SysTick_Millis();
    size_t i = 0;
    while (i < len) {
        int r = S300_UART_TryRead(idx, &buf[i]);
        if (r < 0) return -1;
        if (r == 1) { i++; continue; }
        if (timeout_ms) {
            if ((uint32_t)(S300_SysTick_Millis() - start) >= timeout_ms) break;
        }
    }
    return (int)i;
}

void S300_UART_FlushRx(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    /* Use FCR reset bits if available */
    UART_FCRn(base) = 0x07; /* enable FIFO + reset RX/TX */
}

void S300_UART_FlushTx(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    UART_FCRn(base) = 0x07; /* same as above */
}

void S300_UART_SoftReset(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    /* SRR at 0x88: any write triggers soft reset sequence per DW_apb_uart */
    volatile uint32_t *SRR = (volatile uint32_t *)(base + 0x88u);
    *SRR = 0x7; /* reset UART, RX FIFO, TX FIFO */
}

/* ===== IRQ-driven RX support ===== */

int S300_UART_EnableRxIRQ(uint32_t idx, uint8_t enable)
{
    uint32_t base = UARTn_BASE(idx);
    if (enable) {
        /* Enable FIFO and RX interrupt */
        UART_FCRn(base) = 0x01; /* enable FIFO */
        UART_IERn(base) |= 0x01; /* ERBFI */
    } else {
        UART_IERn(base) &= ~0x01u;
    }
    return 0;
}

size_t S300_UART_RxAvailable(uint32_t idx)
{
    if (idx >= S300_UART_MAX) return 0;
    return s_ring_count(&s_rx_rings[idx]);
}

int S300_UART_RxGet(uint32_t idx, uint8_t *byte)
{
    if (!byte || idx >= S300_UART_MAX) return -1;
    return s_ring_pop(&s_rx_rings[idx], byte) ? 1 : 0;
}

/* Weak IRQ handlers users can override in application if vector table includes them.
   For now, provide names consistent with typical patterns: UART0_IRQHandler, UART3_IRQHandler. */
__attribute__((weak)) void UART0_IRQHandler(void)
{
    uint32_t base = UARTn_BASE(0);
    /* While data ready, push into ring */
    while ((UART_USRn(base) & (1u << 3)) || (UART_LSRn(base) & 1u)) {
        (void)UART_IIRn(base); /* read IIR to acknowledge */
        uint8_t b = (uint8_t)UART_RBRn(base);
        (void)s_ring_push(&s_rx_rings[0], b);
    }
}

__attribute__((weak)) void UART3_IRQHandler(void)
{
    uint32_t base = UARTn_BASE(3);
    while ((UART_USRn(base) & (1u << 3)) || (UART_LSRn(base) & 1u)) {
        (void)UART_IIRn(base);
        uint8_t b = (uint8_t)UART_RBRn(base);
        (void)s_ring_push(&s_rx_rings[3], b);
    }
}

/* ===== Extended features implementations ===== */

int S300_UART_SetFIFO(uint32_t idx, const S300_UartFifoConfig *cfg)
{
    if (!cfg) return -1;
    uint32_t base = UARTn_BASE(idx);
    /* Use Shadow registers if available (SFE/SRT/STET) for clear separation */
    if (cfg->enable) {
        UART_SFEn(base) = 0x1; /* enable FIFO via shadow */
    } else {
        UART_SFEn(base) = 0x0;
    }
    UART_SRTn(base) = (uint32_t)(cfg->rx_trig & 0x3u); /* RX trigger 0..3 */
    /* STET is RO in our soc header; if RO we fallback to FCR write */
    /* Program FCR as well for compatibility (RT[7:6], TET[5:4], DMAM[3], RXRST[1], TXRST[2], FIFOE[0]) */
    uint32_t fcr = 0;
    if (cfg->enable) fcr |= 0x1u;
    if (cfg->dma_mode) fcr |= (1u << 3);
    fcr |= ((uint32_t)(cfg->tx_trig & 0x3u) << 4);
    fcr |= ((uint32_t)(cfg->rx_trig & 0x3u) << 6);
    /* Reset FIFOs to apply new watermarks */
    fcr |= (1u << 1) | (1u << 2);
    UART_FCRn(base) = fcr;
    return 0;
}

int S300_UART_EnableIRQ(uint32_t idx, uint32_t ier_mask)
{
    uint32_t base = UARTn_BASE(idx);
    UART_IERn(base) |= ier_mask;
    return 0;
}

int S300_UART_DisableIRQ(uint32_t idx, uint32_t ier_mask)
{
    uint32_t base = UARTn_BASE(idx);
    UART_IERn(base) &= ~ier_mask;
    return 0;
}

uint32_t S300_UART_GetIIR(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    return UART_IIRn(base);
}

uint32_t S300_UART_GetLSR(uint32_t idx)
{
    uint32_t base = UARTn_BASE(idx);
    return UART_LSRn(base);
}

int S300_UART_SetRS485(uint32_t idx, const S300_UartRS485Config *cfg)
{
    if (!cfg) return -1;
    uint32_t base = UARTn_BASE(idx);
    if (!cfg->enable) {
        /* Disable DE/RE */
        UART_DE_ENn(base) = 0u;
        UART_RE_ENn(base) = 0u;
        UART_TCRn(base) = 0u;
        return 0;
    }
    /* TCR: [3:1] per doc: mode and polarity; bit0 RS485 enable depending on IP, here use bit0 as enable */
    uint32_t tcr = 0;
    /* mode: put into bits [3:2] with our mapping; some IPs use [3] only, we keep backward friendly */
    tcr |= ((uint32_t)(cfg->mode & 0x3u) << 2);
    if (cfg->de_active_high) tcr |= (1u << 2);
    if (cfg->re_active_high) tcr |= (1u << 1);
    tcr |= 1u; /* enable RS485 feature */
    UART_TCRn(base) = tcr;
    /* Enable DE/RE outputs */
    UART_DE_ENn(base) = 1u;
    UART_RE_ENn(base) = 1u;
    /* Program timings */
    uint32_t det = ((uint32_t)cfg->de_deassert_time << 16) | ((uint32_t)cfg->de_assert_time);
    UART_DETn(base) = det;
    uint32_t tat = ((uint32_t)cfg->de2re_turnaround << 16) | ((uint32_t)cfg->re2de_turnaround);
    UART_TATn(base) = tat;
    return 0;
}

int S300_UART_Set9bit(uint32_t idx, const S300_Uart9bitConfig *cfg)
{
    if (!cfg) return -1;
    uint32_t base = UARTn_BASE(idx);
    uint32_t lcr_ext = UART_LCR_EXTn(base);
    if (cfg->enable) lcr_ext |= (1u << 0); else lcr_ext &= ~(1u << 0);
    if (cfg->addr_match_en) lcr_ext |= (1u << 1); else lcr_ext &= ~(1u << 1);
    UART_LCR_EXTn(base) = lcr_ext;
    UART_RARn(base) = (uint32_t)(cfg->rx_addr & 0xFFu);
    UART_TARn(base) = (uint32_t)(cfg->tx_addr & 0xFFu);
    return 0;
}

int S300_UART_9bitSendAddress(uint32_t idx, uint8_t addr)
{
    uint32_t base = UARTn_BASE(idx);
    /* Set TX-ADDR mode: some IPs use LCR_EXT[3] to indicate address frame */
    uint32_t lcr_ext = UART_LCR_EXTn(base);
    lcr_ext |= (1u << 3); /* enter address transmit mode */
    UART_LCR_EXTn(base) = lcr_ext;
    UART_TARn(base) = addr;
    /* send single byte with address flag */
    while (!S300_UART_TxReady(idx)) { /* spin */ }
    UART_THRn(base) = addr;
    /* wait shift out */
    while (!S300_UART_TxIdle(idx)) { /* spin */ }
    /* Exit address mode */
    lcr_ext &= ~(1u << 3);
    UART_LCR_EXTn(base) = lcr_ext;
    return 0;
}
