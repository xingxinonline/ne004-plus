#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "startriscv.h"

#ifndef SPI_BASE
    #define SPI_BASE                0x10000000 // deviceAt0x10000000
#endif
stBoot boot;
unsigned char *pflash = (unsigned char *)SPI_BASE;

int codecpy(char *d, char *b, unsigned int len);
int codecpy4(unsigned int *d, unsigned int *b, unsigned int len);
int cpycode2riscv(stBASE *info);
int cpycode2riscv4(stBASE *info);
// unsigned int memdata[0x190];
int start_riscv(int isstart)
{
    int res = -1;
    unsigned int temp;
    stCODEINFO *head = (stCODEINFO *)0x62200010;//RISCV_CODE_INFO
    if (isstart)
    {
//      init_spi_flash(SPI_FREQ);
//      // read_flash_id(id);
// #if SPI_QUAD==1 || SPI_XIP==1
//      write_flash_cmd(Volatile_SR_Write_Enable);
//      write_flash_status(Write_Status_Register_2,0x2);
//      temp = read_flash_status(Read_Status_Register_2);
//      configure_quad_mode(SPI_XIP_FREQ);
// #endif
        //开启RISC v电源，复位RISC v
        RISC_EN &= ~(1 << 6);
        //arm使能访问RISC v的SRAM空间
        ARM_EN_RISCV_RAM |= 1;
        //从flash中读取程序信息 head信息
        // codecpy((char *)&boot.head,(char *)pflash,sizeof(stBOOTGEN));
        codecpy4((unsigned int *)&boot.head, (unsigned int *)pflash, sizeof(stBOOTGEN) / 4);
        //复制flash中的程序到RISC v的SRAM中
        // pflash+=0x460;
        // codecpy4(memdata,(unsigned int *)pflash,0x190 /4);
        // while(1);
        res = -1;
        //简单检查head头信息是否存在
        if (((unsigned int)(boot.head.riscv.exe_addr & (~(0x3FFFF))) == (unsigned int)0x80000000) && (boot.head.riscv.len < (unsigned int)0x40000))
        {
            temp = (unsigned int)0x80040000 - boot.head.riscv.exe_addr;
            if (boot.head.riscv.len < temp)
                res = 0;
        }
        if (res == 0)
        {
            //拷贝数据从flash中到iram中
            // res = cpycode2riscv(&boot.head.riscv);
            res = cpycode2riscv4(&boot.head.riscv);
            if (res == 0)
            {
                head->addr = boot.head.riscv.exe_addr;
                head->len = boot.head.riscv.len;
                head->check = boot.head.riscv.check;
                head->pro = boot.head.riscv.pro;
                //启动RISC v，释放复位信号
                ARM_EN_RISCV_RAM &= ~1;
                RISC_EN |= (1 << 6);
            }
        }
        ARM_EN_RISCV_RAM &= ~1;
    }
    return res;
}

int codecpy(char *d, char *b, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        d[i] = b[i];
    }
    return i;
}

int codecpy4(unsigned int *d, unsigned int *b, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++)
    {
        d[i] = b[i];
    }
    return i;
}
void *memcpy(void *__restrict d, const void *__restrict b, size_t len)
{
    unsigned int i;
    unsigned char *pdst = (unsigned char *)d;
    unsigned char *psrc = (unsigned char *)b;
    for (i = 0; i < len; i++)
    {
        pdst[i] = psrc[i];
    }
    return (void *)i;
}

int cpycode2riscv(stBASE *info)
{
    int res = -1;
    unsigned int i, j;
    unsigned int addr = info->exe_addr >> 5;
    unsigned int offset = info->exe_addr & 0x1F;
    unsigned char *pdst = (unsigned char *)0x62200020;//ARM_CPY_RISCV_RAM_BASE
    unsigned char *psrc = (unsigned char *)info->addr + SPI_BASE;
    for (i = 0; i < info->len; i += 32)
    {
        ARM_SET_RISCV_RAM_BASE = addr;
        addr++;
        for (j = offset; j < 32; j++)
        {
            pdst[j] = *psrc++;
        }
        offset = 0;
    }
    if (i == info->len)
        res = 0;
    return res;
}

int cpycode2riscv4(stBASE *info)
{
    int res = -1;
    unsigned int i, j;
    unsigned int addr = info->exe_addr >> 5;
    unsigned int offset = info->exe_addr & 0x1F;
    unsigned int *pdst = (unsigned int *)0x62200020;//ARM_CPY_RISCV_RAM_BASE
    unsigned int *psrc = (unsigned int *)(info->addr + SPI_BASE);
    unsigned int tempdata[8];
    // unsigned int index = 0;
    offset >>= 2;
    for (i = 0; i < info->len; i += 32)
    {
        ARM_SET_RISCV_RAM_BASE = addr;
        addr++;
        //read from flash to arm mem
        codecpy4(tempdata, psrc, (8 - offset));
        psrc += (8 - offset);
        //cp arm mem to riscv mem
        for (j = offset; j < (8 - offset); j++)
        {
            pdst[j] = tempdata[j];
        }
        offset = 0;
    }
    offset = ((info->len % 32) == 0) ? info->len : (info->len + 32 - (info->len % 32));
    if (i == offset)
        res = 0;
    return res;
}
