#ifndef S300_I2C_S300_H
#define S300_I2C_S300_H

#ifdef __cplusplus
extern "C" {
#endif

#include "s300.h"

/* CMSIS-style I2C register map (DW_apb_i2c compatible) */
typedef struct
{
    volatile uint32_t CON;            /* 0x00: Control */
    volatile uint32_t TAR;            /* 0x04: Target address */
    volatile uint32_t SAR;            /* 0x08: Slave address */
    volatile uint32_t HS_MADDR;       /* 0x0C: High speed master code */
    volatile uint32_t DATA_CMD;       /* 0x10: TX/RX data and command */
    volatile uint32_t SS_SCL_HCNT;    /* 0x14 */
    volatile uint32_t SS_SCL_LCNT;    /* 0x18 */
    volatile uint32_t FS_SCL_HCNT;    /* 0x1C */
    volatile uint32_t FS_SCL_LCNT;    /* 0x20 */
    volatile uint32_t HS_SCL_HCNT;    /* 0x24 */
    volatile uint32_t HS_SCL_LCNT;    /* 0x28 */
    volatile uint32_t INTR_STAT;      /* 0x2C */
    volatile uint32_t INTR_MASK;      /* 0x30 */
    volatile uint32_t RAW_INTR_STAT;  /* 0x34 */
    volatile uint32_t RX_TL;          /* 0x38 */
    volatile uint32_t TX_TL;          /* 0x3C */
    volatile uint32_t CLR_INTR;       /* 0x40 */
    volatile uint32_t CLR_RX_UNDER;   /* 0x44 */
    volatile uint32_t CLR_RX_OVER;    /* 0x48 */
    volatile uint32_t CLR_TX_OVER;    /* 0x4C */
    volatile uint32_t CLR_RD_REQ;     /* 0x50 */
    volatile uint32_t CLR_TX_ABRT;    /* 0x54 */
    volatile uint32_t CLR_RX_DONE;    /* 0x58 */
    volatile uint32_t CLR_ACTIVITY;   /* 0x5C */
    volatile uint32_t CLR_STOP_DET;   /* 0x60 */
    volatile uint32_t CLR_START_DET;  /* 0x64 */
    volatile uint32_t CLR_GEN_CALL;   /* 0x68 */
    volatile uint32_t ENABLE;         /* 0x6C */
    volatile uint32_t STATUS;         /* 0x70 */
    volatile uint32_t TXFLR;          /* 0x74 */
    volatile uint32_t RXFLR;          /* 0x78 */
    volatile uint32_t SDA_HOLD;       /* 0x7C */
    volatile uint32_t TX_ABRT_SOURCE; /* 0x80 */
    volatile uint32_t SLV_DATA_NACK_ONLY; /* 0x84 */
    volatile uint32_t DMA_CR;         /* 0x88 */
    volatile uint32_t DMA_TDLR;       /* 0x8C */
    volatile uint32_t DMA_RDLR;       /* 0x90 */
    volatile uint32_t SDA_SETUP;      /* 0x94 */
    volatile uint32_t ACK_GENERAL_CALL; /* 0x98 */
    volatile uint32_t ENABLE_STATUS;  /* 0x9C */
    volatile uint32_t FS_SPKLEN;      /* 0xA0 */
    volatile uint32_t HS_SPKLEN;      /* 0xA4 */
    volatile uint32_t CLR_RESTART_DET;/* 0xA8 */
    uint32_t RESERVED0[18];           /* 0xAC..0xF0 */
    volatile uint32_t COMP_PARAM_1;   /* 0xF4 */
    volatile uint32_t COMP_VERSION;   /* 0xF8 */
    volatile uint32_t COMP_TYPE;      /* 0xFC */
} S300_I2C_TypeDef;

#define I2C0   ((S300_I2C_TypeDef *)I2C0_BASE)
#define I2C1   ((S300_I2C_TypeDef *)I2C1_BASE)
#define I2C2   ((S300_I2C_TypeDef *)I2C2_BASE)
#define I2C3   ((S300_I2C_TypeDef *)I2C3_BASE)

#ifdef __cplusplus
}
#endif

#endif /* S300_I2C_S300_H */
