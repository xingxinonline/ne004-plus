/*
 * @Author       : panxinhao
 * @Date         : 2023-07-25 11:04:26
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-08 15:02:53
 * @FilePath     : \\ne004-plus\\cortexm4_default\\main.c
 * @Description  :
 *
 * Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ne004xx.h"
#include "startriscv.h"

#include "riscv_dma.h"
#include "ne004xx_uart.h"
#include "servo.h"

#define ARM_RISCV_IPCM_KWS      0x622000F0U
#define ARM_RISCV_IPCM_FFT      0x622000F8U
#define ARM_RISCV_IPCM_END      0x622000FCU

#define ARM_RISCV_STOP_VALUE    0x5A5A5A5AU

#define PDM_DMA_USE             DMA1
#define PDM_DMA_WRITE_CHANNEL   DMA_CH1

#include <stdio.h>

#define PUTCHAR_PROTOTYPE       int __io_putchar(int ch)

PUTCHAR_PROTOTYPE
{
    while (UART_STAT(UART0) & UART_STAT_TXFL);
    UART_DATA(UART0) = (uint8_t)ch;
    return ch;
}

int _write(int fd, const char *buf, int nbytes)
{
    (void)fd;
    for (int i = 0; i < nbytes; i++)
    {
        /* code */
        __io_putchar(*buf++);
    }
    return nbytes;
}

void Init_DWT_Clock(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; //Enable DWT
    DWT->CYCCNT = 0; //Clear the cycle counter
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //Enable the cycle counter
}

uint32_t volatile start_cycles;
uint32_t volatile end_cycles;
uint32_t volatile nop_cycles;

static void __attribute__((optimize("O0"))) arm_stop(void)
{
    REG32(ARM_RISCV_IPCM_END) = ARM_RISCV_STOP_VALUE;
}
volatile uint32_t  systick_cnt = 0; // must be volatile to prevent compiler optimisations

void disableInterrupts()
{
    __disable_irq();
}

void enableInterrupts()
{
    __enable_irq();
}

volatile uint8_t rx_start_flag = 0;
volatile uint8_t rx_timeout_flag = 0;
volatile uint32_t rx_data_last = 0;

struct ring_buffer_t
{
    /** Index of head. */
    volatile uint32_t head_index;
    /** Index of tail. */
    volatile uint32_t tail_index;
    /** Buffer memory. */
    uint64_t buffer;
};

#define RING_BUFFER_SIZE 0x10000
#define RING_BUFFER_MASK (RING_BUFFER_SIZE - 1)

typedef struct ring_buffer_t ring_buffer_t;


static volatile ring_buffer_t *npu_buffer = (struct ring_buffer_t *)0x60000000U;
static volatile uint8_t pdm_avail_flag = 0;
static volatile uint8_t kws_avail_flag = 0;
static volatile uint8_t kws_avail_enable = 0;

#define PDM_DATA_WIDTH          0x100U //number of 16bit
#define PDM_BYTES               (PDM_DATA_WIDTH * 2)    //number of bytes when pdm avail
#define PDM_AVAIL_REPEAT        (0x2000U / PDM_DATA_WIDTH)  //number of pdm repeat
#define PDM_SRAM_REPEAT         (0x8000U / PDM_BYTES)        //32KB sram
#define PDM_NPU_SRAM_REPEAT     (0x20000U / PDM_BYTES)    //128KB sram

#define NUM_SERVOS 4
ServoState servos[NUM_SERVOS];

void  SysTick_Handler(void)
{
    systick_cnt++;
    if (rx_start_flag)
    {
        /* code */
        if ((systick_cnt - rx_data_last) >= 100)
        {
            /* code */
            rx_timeout_flag = 1;
            rx_start_flag = 0;
        }
    }
    if ((systick_cnt % 500) == 0)
    {
        /* code */
        REG32(0x40006000) = ~REG32(0x40006000);
    }
    if (systick_cnt % 10 == 0)
    {
        /* code */
        for (size_t i = 0; i < NUM_SERVOS; i++)
        {
            /* code */
            servos[i].current_ticks = systick_cnt;
        }
    }
}

void UpdateServos(void)
{
    // 按优先级顺序检查电机
    for (int i = 0; i < NUM_SERVOS; i++)
    {
        ServoState *servo = &servos[i];
        if (servo->isMoving && (servo->current_ticks - servo->lastUpdate >= 20))    // 每10ms更新一次
        {
            int previousAngle = servo->currentAngle;
            if (servo->currentAngle < servo->targetAngle)
            {
                servo->currentAngle += servo->stepSize;
                if (servo->currentAngle > servo->targetAngle)
                {
                    servo->currentAngle = servo->targetAngle;
                }
            }
            else if (servo->currentAngle > servo->targetAngle)
            {
                servo->currentAngle -= servo->stepSize;
                if (servo->currentAngle < servo->targetAngle)
                {
                    servo->currentAngle = servo->targetAngle;
                }
            }
            // 只有在角度发生变化时才更新PWM信号
            if (servo->currentAngle != previousAngle)
            {
                set_pwm_angle(servo, servo->currentAngle);
            }
            // 更新上次更新时间
            servo->lastUpdate = servo->current_ticks;
            // 检查是否已经到达目标角度
            if (servo->currentAngle == servo->targetAngle)
            {
                servo->isMoving = 0;  // 停止移动
            }
            if (servo->isMoving)
            {
                /* code */
                // 一旦找到正在移动的电机，跳过其他电机
                return;
            }
        }
    }
}


uint8_t rxover_flag = 0;
uint8_t txover_flag = 0;

uint8_t tx_flag = 0;

uint8_t rx_data_buf[128] = {0};
volatile size_t rx_data_cnt = 0;

uint8_t cmd_return[128];
int LEFT1_SPEED_NOW = 0;
int LEFT2_SPEED_NOW = 0;
int RIGHT1_SPEED_NOW = 0;
int RIGHT2_SPEED_NOW = 0;

int max_speed = 400;
int max_neg_speed = -400;

void shortToAscii(short number, char *buffer, int maxDigits)
{
    int i = 0, j;
    char temp[16] = { 0 }; // 用于存储数字的中间转换结果
    // 提取数字的每一位并转换为字符
    do
    {
        temp[i++] = (number % 10) + '0'; // 将数字转换为字符
        number /= 10;
    }
    while (number > 0 && i < maxDigits);
    if (i < 4)
    {
        for (; i < 4; i++)
        {
            temp[i] = '0';
        }
    }
    // 逆序存储结果到 buffer 中，最多存储 maxDigits 位
    for (j = 0; j < maxDigits; j++)
    {
        buffer[j] = temp[--i];
    }
    // 添加字符串结束符
    buffer[j] = '\0';
}

void print_to_car(char *s, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        __io_putchar(s[i]);
    }
}
void umemset(char *s, char v, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        s[i] = v;
    }
}
//参数 左轮速度和右轮速度 范围 -1000 到 1000
void car_set(int car_left1, int car_right1, int car_left2, int car_right2)
{
    //总线马达设置
    // sprintf((char *)cmd_return, "{#006P%04dT0000!#007P%04dT0000!#008P%04dT0000!#009P%04dT0000!}\n",
    //  (int)(1500+car_left1), (int)(1500-car_right1),(int)(1500+car_left2), (int)(1500-car_right2));
    // //zx_uart_send_str(cmd_return);
    // printf("{#006P%04dT0000!#007P%04dT0000!#008P%04dT0000!#009P%04dT0000!}\n",
    //  (int)(1500+car_left1), (int)(1500-car_right1),(int)(1500+car_left2), (int)(1500-car_right2));
    // int i;
    char *buf0 = "#006P";
    char *buf1 = "T0000!#007P";
    char *buf2 = "T0000!#008P";
    char *buf3 = "T0000!#009P";
    char *buf4 = "T0000!";
    //print_to_car(buf0, (sizeof(buf0) + 1));
    print_to_car(buf0, 5);
    short val0 = (short)(1500 + car_left1);
    short val1 = (short)(1500 - car_right1);
    short val2 = (short)(1500 + car_left2);
    short val3 = (short)(1500 - car_right2);
    char data[16] = {0};
    umemset(data, 0, 16);
    shortToAscii(val0, data, 4);
    print_to_car(data, 4);
    //print_to_car(buf1,sizeof(buf1));
    print_to_car(buf1, 11);
    umemset(data, 0, 16);
    shortToAscii(val1, data, 4);
    print_to_car(data, 4);
    print_to_car(buf2, 11);
    umemset(data, 0, 16);
    shortToAscii(val2, data, 4);
    print_to_car(data, 4);
    print_to_car(buf3, 11);
    umemset(data, 0, 16);
    shortToAscii(val3, data, 4);
    print_to_car(data, 4);
    print_to_car(buf4, 6);
    // print_to_car("\n\r",2);
    return;
}

void car_soft_set(int car_left1, int car_right1, int car_left2, int car_right2)
{
    while ((car_left1 != LEFT1_SPEED_NOW) | (car_right1 != RIGHT1_SPEED_NOW) |
            (car_left2 != LEFT2_SPEED_NOW) | (car_right2 != RIGHT2_SPEED_NOW))
    {
        if (LEFT1_SPEED_NOW < car_left1)
        {
            LEFT1_SPEED_NOW += 100;
        }
        else if (LEFT1_SPEED_NOW > car_left1)
        {
            LEFT1_SPEED_NOW -= 100;
        }
        else
        {
            LEFT1_SPEED_NOW = LEFT1_SPEED_NOW;
        }
        if (RIGHT1_SPEED_NOW < car_right1)
        {
            RIGHT1_SPEED_NOW += 100;
        }
        else if (RIGHT1_SPEED_NOW > car_right1)
        {
            RIGHT1_SPEED_NOW -= 100;
        }
        else
        {
            RIGHT1_SPEED_NOW = RIGHT1_SPEED_NOW;
        }
        if (LEFT2_SPEED_NOW < car_left2)
        {
            LEFT2_SPEED_NOW += 100;
        }
        else if (LEFT2_SPEED_NOW > car_left2)
        {
            LEFT2_SPEED_NOW -= 100;
        }
        else
        {
            LEFT2_SPEED_NOW = LEFT2_SPEED_NOW;
        }
        if (RIGHT2_SPEED_NOW < car_right2)
        {
            RIGHT2_SPEED_NOW += 100;
        }
        else if (RIGHT2_SPEED_NOW > car_right2)
        {
            RIGHT2_SPEED_NOW -= 100;
        }
        else
        {
            RIGHT2_SPEED_NOW = RIGHT2_SPEED_NOW;
        }
        car_set(LEFT1_SPEED_NOW, RIGHT1_SPEED_NOW, LEFT2_SPEED_NOW, RIGHT2_SPEED_NOW);
        // arm_delay_ms(10);
    }
}

int main(void)
{
    uint32_t temp = 0x0U;
    Init_DWT_Clock();
    start_cycles = DWT->CYCCNT;
    // NOP指令
    __NOP();
    end_cycles = DWT->CYCCNT;
    nop_cycles = end_cycles - start_cycles;
    arm_stop();
    /* disable pdm */
    REG32(0x4000D08CU) = 0x0U;
    REG32(0x4000D05CU) = 0x2U;
    /* disable all interrupt */
    disableInterrupts();
    REG32(0x4000D0F8U) = 0x0U;  //屏蔽riscv所有外部中断
    REG32(0x4000D0FCU) = 0x0U;  //屏蔽riscv2arm所有中断
    REG64(0x4000D174U) = 0x0U;  //屏蔽arm所有个中断
    REG64(0x4000D17CU) = 0x0U;  //屏蔽arm2riscv所有中断
    npu_buffer->head_index = 0;
    /* config IO */
    PAD_GP0_FUNCSEL = 0x1;
    REG32(0x40006004) = 0xFFFFFFFF;
    PAD_GP1_FUNCSEL |= (0x4 << 4); //pwm
    PAD_GP2_FUNCSEL |= (0x4 << 8);
    PAD_GP3_FUNCSEL |= (0x4 << 12);
    PAD_GP4_FUNCSEL |= (0x4 << 16);
    PAD_GP7_FUNCSEL  |= (0x3 << 28); // JTAG
    PAD_GP8_FUNCSEL  |= (0x3);
    PAD_GP9_FUNCSEL  |= (0x3 << 4);
    PAD_GP10_FUNCSEL |= (0x03 << 8);
    PAD_GP13_FUNCSEL |= (0x01 << 20); // riscv UART
    PAD_GP14_FUNCSEL |= (0x1 << 24);
    PAD_GP15_FUNCSEL |= (0x02 << 28); // ARM UART
    PAD_GP16_FUNCSEL |= (0x2);
    start_riscv(1);
    /* enable riscv interrupt parameters */
    REG32(0x4000D0F8U) |= 0x1U << RISCV_ARM2RISCV_IRQ;
    REG32(0x4000D0F8U) |= 0x1U << RISCV_DMA1_IRQ;
    REG32(0x4000D0F8U) |= 0x1U << RISCV_ARM_NOTICE_IRQ;
    /* enable arm interrupt parameters */
    REG64(0x4000D174U) |= 1ULL << PDM_AVAIL_IRQn;   //允许pdm2pcm中断
    REG64(0x4000D174U) |= 1ULL << RISCV_NOTICE_IRQn;
    REG64(0x4000D174U) |= 1ULL << RXOVRINT0_IRQn;   //允许pdm2pcm中断
    REG64(0x4000D174U) |= 1ULL << TXOVRINT0_IRQn; //允许riscv_notice中断
    REG64(0x4000D174U) |= 1ULL << RXINT0_IRQn; //允许riscv_notice中断
    REG64(0x4000D174U) |= 1ULL << TXINT0_IRQn;
    //允许riscv_notice中断
    /* enable arm2riscv interrupt */
    REG64(0x4000D17CU) |= 1ULL << REQ_FFT_DONE_IRQn; //发送FFT中断到riscv处理
    /* enable arm nvic irq */
    NVIC_EnableIRQ(PDM_AVAIL_IRQn);
    NVIC_EnableIRQ(RISCV_NOTICE_IRQn);
    NVIC_EnableIRQ(RXOVRINT0_IRQn);
    NVIC_EnableIRQ(TXOVRINT0_IRQn);
    NVIC_EnableIRQ(RXINT0_IRQn);
    NVIC_EnableIRQ(TXINT0_IRQn);
    UART_BAUD(UART0) = UART_BAUD_DIV & (APBClock / 9600);
    UART_CTRL(UART0) |= UART_CTRL_TEN | UART_CTRL_REN | UART_CTRL_TIE | UART_CTRL_RIE | UART_CTRL_TORIE | UART_CTRL_RORIE;
    //发送测试字节
    // UART_DATA(UART0) = 'A';
    riscv_dma_enable(PDM_DMA_USE);
    riscv_dma_init(PDM_DMA_USE, PDM_DMA_WRITE_CHANNEL, 0x1000, 8, 8);
    riscv_dma_interrupt_disable(PDM_DMA_USE, PDM_DMA_WRITE_CHANNEL);
    /* enable pdm */
    REG32(0x4000D100U) = PDM_DATA_WIDTH;// 配置params_pdm_width
    REG32(0x4000D08CU) = 0x1U;
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Hello from NE004-PLUS cortex-m4 core!\r\n");
    SysTick_Config(AHBClock / 1000); // set tick to every 1ms
    pwm_init(PWM0);
    pwm_init(PWM1);
    servo_init(&servos[0], PWM0_BASE, PWM0_CH2_BASE, 90); //夹紧、张开
    servo_init(&servos[1], PWM0_BASE, PWM0_CH3_BASE, 90); //左转、右转
    servo_init(&servos[2], PWM1_BASE, PWM1_CH1_BASE, 90); //上升、下降
    servo_init(&servos[3], PWM1_BASE, PWM1_CH2_BASE, 90); //后缩、前伸
    enableInterrupts();
    while (1)
    {
        if (pdm_avail_flag)
        {
            // UART_DATA(UART0) = 'B';
            static size_t pdm_avail_cnt = 0;
            pdm_avail_flag = 0;
            riscv_dma_start(PDM_DMA_USE, PDM_DMA_WRITE_CHANNEL, 0x46000000 + PDM_BYTES * (pdm_avail_cnt % PDM_BYTES), 0x60100000 + PDM_BYTES * (pdm_avail_cnt % PDM_NPU_SRAM_REPEAT));
            while (DMA_CHEN(PDM_DMA_USE) & (DMA_CHEN_CH_EN & (0x01ULL << (PDM_DMA_WRITE_CHANNEL - 1))));
            npu_buffer->head_index = ((npu_buffer->head_index + PDM_DATA_WIDTH) & RING_BUFFER_MASK);
            pdm_avail_cnt++;
            if ((pdm_avail_cnt % 8) == 0)
            {
                /* code */
                arm_stop();
            }
        }
        if (kws_avail_enable && kws_avail_flag)
        {
            /* code */
            uint32_t cmd_kws = 0;
            cmd_kws = kws_avail_flag;
            kws_avail_flag = 0;
            
            switch (cmd_kws)
            {
            case 1:
                /* code */
                servo_set_angle(&servos[0], 90); // reset
                servo_set_angle(&servos[1], 90); //
                servo_set_angle(&servos[2], 90); //
                servo_set_angle(&servos[3], 90); //
                printf("reset\r\n");
                break;
            case 2:
                /* code */
                servo_set_angle(&servos[3], 110); // 前伸
                printf("qian jing\r\n");
                break;
            case 3:
                /* code */
                servo_set_angle(&servos[3], 60); // 后缩
                printf("hou tui\r\n");
                break;
            case 4:
                /* code */
                servo_set_angle(&servos[1], 135); // 左转
                printf("zuo zhuan\r\n");
                break;
            case 5:
                /* code */
                servo_set_angle(&servos[1], 45); // 右转
                printf("you zhuan\r\n");
                break;
            case 6:
                /* code */
                servo_set_angle(&servos[2], 135); // 上升
                printf("shang sheng\r\n");
                break;
            case 7:
                /* code */
                servo_set_angle(&servos[2], 60); // 下降
                printf("xia jiang\r\n");
                break;
            case 8:
                /* code */
                servo_set_angle(&servos[0], 0); // 夹紧
                printf("zhua qu\r\n");
                break;
            case 9:
                /* code */
                servo_set_angle(&servos[0], 135); // 张开
                printf("zhang kai\r\n");
                break;
            default:
                break;
            }
        }
        if (rx_timeout_flag)
        {
            /* code */
            rx_timeout_flag = 0;
            switch (rx_data_buf[0])
            {
            case 0x13://resey
                /* code */
                servo_set_angle(&servos[0], 90); // reset
                servo_set_angle(&servos[1], 90); //
                servo_set_angle(&servos[2], 90); //
                servo_set_angle(&servos[3], 90); //
                printf("reset\r\n");
                break;
            case 0x14://上升
                /* code */
                servo_set_angle(&servos[2], 135); // 上升
                printf("shang sheng\r\n");
                break;
            case 0x15://下降
                /* code */
                servo_set_angle(&servos[2], 60); // 下降
                printf("xia jiang\r\n");
                break;
            case 0x16://夹紧
                /* code */
                servo_set_angle(&servos[0], 0); // 夹紧
                printf("zhua qu\r\n");
                break;
            case 0x17://张开
                /* code */
                servo_set_angle(&servos[0], 135); // 张开
                printf("zhang kai\r\n");
                break;
            case 0x18://后缩
                /* code */
                servo_set_angle(&servos[3], 60); // 后缩
                printf("hou tui\r\n");
                break;
            case 0x19://前伸
                /* code */
                servo_set_angle(&servos[3], 110); // 前伸
                printf("qian jing\r\n");
                break;
            case 0x1A://左转
                /* code */
                servo_set_angle(&servos[1], 135); // 左转
                printf("zuo zhuan\r\n");
                break;
            case 0x1B://右转
                /* code */
                servo_set_angle(&servos[1], 45); // 右转
                printf("you zhuan\r\n");
                break;
            /* car control */
            case 0x20:  //car qian jin
                car_soft_set(max_speed - 200, max_speed - 200, max_speed - 200, max_speed - 200);
                break;
            case 0x21:  //car hou tui
                car_soft_set(max_neg_speed + 200, max_neg_speed + 200, max_neg_speed + 200, max_neg_speed + 200);
                break;
            case 0x22:  //car zuo yi
                car_soft_set(max_speed, max_neg_speed, max_neg_speed, max_speed);
                break;
            case 0x23:  //car you yi
                car_soft_set(max_neg_speed, max_speed, max_speed, max_neg_speed);
                break;
            case 0x24:  //car ting zhi
                car_soft_set(0, 0, 0, 0);
                break;
            case 0x25:  //car zuo qian
                car_soft_set(0, max_speed, max_speed, 0);
                break;
            case 0x26:  //car you qian
                car_soft_set(max_speed, 0, 0, max_speed);
                break;
            case 0x27:  //car zuo hou
                car_soft_set(max_neg_speed, 0, 0, max_neg_speed);
                break;
            case 0x28:  //car you hou
                car_soft_set(0, max_neg_speed, max_neg_speed, 0);
                break;
            case 0x29:  //car xuan zhuan
                car_soft_set(max_speed - 100, max_neg_speed + 100, max_speed - 100, max_neg_speed + 100);
                break;
            case 0x30:  //disable voice
                kws_avail_enable = 0;
                break;
            case 0x31:  //enable voice
                kws_avail_enable = 1;
                break;
            default:
                break;
            }
            for (size_t i = 0; i < rx_data_cnt; i++)
            {
                /* code */
                __io_putchar(rx_data_buf[i]);
            }
            rx_data_cnt = 0;
        }
        UpdateServos(); // 更新舵机状态
    }
    (void)temp;
    return 0;
}

uint32_t value = 0;
void RXINT0_IRQHandler(void)
{
    if (rx_start_flag == 0)
    {
        /* code */
        rx_start_flag = 1;
    }
    rx_data_last = systick_cnt;
    if ((UART_STAT(UART0) & UART_STAT_RXFL) != 0)
    {
        /* code */
        uint8_t temp = UART_DATA(UART0);
        rx_data_buf[rx_data_cnt++] = temp;
    }
    UART_INTSTAT(UART0) |= UART_INTSTAT_RIE;
}

void TXINT0_IRQHandler(void)
{
    tx_flag = 1;
    UART_INTSTAT(UART0) |= UART_INTSTAT_TIE;
}

void RXOVRINT0_IRQHandler(void)
{
    rxover_flag = 1;
    UART_INTSTAT(UART0) |= UART_INTSTAT_RORIE;
}

void TXOVRINT0_IRQHandler(void)
{
    txover_flag = 1;
    UART_INTSTAT(UART0) |= UART_INTSTAT_TORIE;
}

void PDM_AVAIL_IRQHandler(void)
{
    static size_t pdm_isr_cnt = 0;
    // ARM清pdm_avail中断, 配置地址32‘h4000_d104, 配置数据32’h0000_0001。
    REG32(0x4000D104U) = 0x00000001U;
    // ARM清pdm_avail中断, 配置地址32‘h4000_d104, 配置数据32’h0000_0000。
    REG32(0x4000D104U) = 0x00000000U;
    pdm_isr_cnt++;
    REG32(0x4000D100U) = PDM_DATA_WIDTH + PDM_DATA_WIDTH * (pdm_isr_cnt % PDM_AVAIL_REPEAT);// 配置params_pdm_width
    pdm_avail_flag = 1;
}

void RISCV_NOTICE_IRQHandler(void)
{
#define FFT_HARDWARE_START      (1U << 0)
#define FFT_HARDWARE_END        (1U << 1)
    //TODO 清除riscv notice中断
    REG32(0x4000D194U) = 0x1U;
    REG32(0x4000D194U) = 0x0U;
    uint32_t temp = REG32(ARM_RISCV_IPCM_FFT);
    if (temp & FFT_HARDWARE_START)
    {
        //TODO 启动fft
        REG32(0x4000D0ACU) = 0x1U;
        //TODO 清除IPCM
        temp &= ~FFT_HARDWARE_START;
        REG32(ARM_RISCV_IPCM_FFT) = temp;
        // printf("FFT start\r\n");
    }
    else if (temp & FFT_HARDWARE_END)
    {
        //TODO 停止fft
        REG32(0x4000D0ACU) = 0x0U;
        //TODO 清除IPCM
        temp &= ~FFT_HARDWARE_END;
        REG32(ARM_RISCV_IPCM_FFT) = temp;
        // printf("FFT end\r\n");
    }
    temp = REG32(ARM_RISCV_IPCM_KWS);
    if (temp)
    {
        /* code */
        REG32(ARM_RISCV_IPCM_KWS) = 0;
        kws_avail_flag = temp;
    }
}

void arm_delay_us(uint32_t us)
{
    uint32_t Delay = us * (APBClock / 1000000U) / 4;
    do
    {
        __NOP();
    }
    while (Delay --);
}
