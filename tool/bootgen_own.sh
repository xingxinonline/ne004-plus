#!/bin/sh
echo "create flash.bin"
../tool/bootgen $1_flash.bin $1.bin $2.bin
#swp bin
echo "create flashswp.bin"
../tool/swapbin $1_flashswp.bin $1_flash.bin
#general img
#echo "generate flash.img"
#./tool/imagegen bin/flashswp.img bin/flashswp.bin 0x40000

#check
#echo "check flash.img"
#./tool/checkimg bin/flashswp.img

#raw bin
#echo "create flash.txt"
#cp -a bin/flash.bin bin/flashswp.bin
#./tool/bin2txt bin/flash.txt bin/flashswp.bin
#./tool/programflash bin/MEM.TXT bin/flash.txt
echo "boot done"
