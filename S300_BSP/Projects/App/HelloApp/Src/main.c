#include "s300.h"
#include <stdio.h>
#ifndef UART_DEBUG_IDX
#define UART_DEBUG_IDX 3u
#endif
static void minimal_uart3_gpio_init(void){const uint32_t RCC_BASE=0x4000A000u;const uint32_t APB1_CLK_EN_OFF=0x000Cu;volatile uint32_t*const APB1_CLK_EN=(uint32_t*)(RCC_BASE+APB1_CLK_EN_OFF);*APB1_CLK_EN|=(1u<<8)|(1u<<3);const uint32_t IO_MATRIX_BASE=0x40008000u;volatile uint32_t*const IO_MATRIX_CFG1=(uint32_t*)(IO_MATRIX_BASE+4u);uint32_t v=*IO_MATRIX_CFG1;v&=~((0x3u<<20)|(0x3u<<22));v|=((0x3u<<20)|(0x3u<<22));*IO_MATRIX_CFG1=v;}
static void uart_set_baud(uint32_t base,uint32_t baud){uint32_t clk=SystemCoreClock;if(baud==0u||clk==0u)return;uint32_t denom=baud*16u;uint32_t div=clk/denom;uint32_t rem=clk%denom;uint32_t dlf=(uint32_t)((uint64_t)rem*16u+(denom/2u))/denom;if(dlf>=16u){dlf=0u;div+=1u;}if(div==0u)div=1u;UART_LCRn(base)|=0x80;UART_DLLn(base)=div&0xFFu;UART_DLHn(base)=(div>>8)&0xFFu;UART_LCRn(base)&=~0x80u;UART_DLFn(base)=dlf&0x0Fu;}
static void uart_init_poll(uint32_t idx,uint32_t baud){uint32_t base=UARTn_BASE(idx);UART_IERn(base)=0x0u;UART_FCRn(base)=0x07u;UART_LCRn(base)=0x03u;UART_MCRn(base)=0x00u;uart_set_baud(base,115200u);} 
int main(void){minimal_uart3_gpio_init();uart_init_poll(UART_DEBUG_IDX,115200u);setvbuf(stdout,NULL,_IONBF,0);printf("[App] Hello from App!\n");for(;;){__WFI();}}
