#ifndef _START_RISCV_H_
#define _START_RISCV_H_

#include "ne004xx.h"

typedef struct _code_info_
{
    unsigned int addr;//RISCV RAM CODE EXE ADDR
    unsigned int len;//CODE SIZE
    unsigned int check;//CHECK VALUE
    unsigned int pro;//PRO
} stCODEINFO;

typedef struct _base_
{
    unsigned int addr;// 0
    unsigned int len;// 4
    unsigned int check; //8
    unsigned int pro;//0xC
    unsigned int exe_addr;//0x10
    unsigned char version[12];//0x14
} stBASE;

typedef struct _bootgen_
{
    stBASE arm;
    stBASE riscv;
    stBASE conf;
} stBOOTGEN;

typedef struct _boot_
{
    stBOOTGEN head;
} stBoot;

extern stBoot boot;

int start_riscv(int isstart);
#endif // !_START_RISCV_H_
