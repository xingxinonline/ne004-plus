#!/bin/sh
###
 # @Author       : panxinhao
 # @Date         : 2023-05-29 13:44:10
 # @LastEditors  : panxinhao
 # @LastEditTime : 2023-08-22 16:49:21
 # @FilePath     : \\testbench_base\\tool\\bootgen.sh
 # @Description  : 
 # 
 # Copyright (c) 2023 by xinhao.pan@pimchip.cn, All Rights Reserved. 
### 

echo "create flash.bin"
./tool/bootgen bin/flash.bin $1/out/$1.bin $2/out/$2.bin
#swp bin
echo "create flashswp.bin"
./tool/swapbin bin/flashswp.bin bin/flash.bin
#general img
#echo "generate flash.img"
#./tool/imagegen bin/flashswp.img bin/flashswp.bin 0x40000

#check
#echo "check flash.img"
#./tool/checkimg bin/flashswp.img

#raw bin
#echo "create flash.txt"
#cp -a bin/flash.bin bin/flashswp.bin
./tool/bin2txt bin/flash.txt bin/flashswp.bin

cp -a docs/MEM.TXT bin/MEM.TXT
./tool/programflash bin/MEM.TXT bin/flash.txt

echo "boot done"
