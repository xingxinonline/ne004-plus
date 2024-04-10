#!/bin/sh
echo "create flash.bin"
./tool/bootgen.exe bin/flash.bin $1/out/$1.bin $2/out/$2.bin
#swp bin
echo "create flashswp.bin"
./tool/swapbin.exe bin/flashswp.bin bin/flash.bin
#general img
#echo "generate flash.img"
#./tool/imagegen.exe bin/flashswp.img bin/flashswp.bin 0x40000

#check
#echo "check flash.img"
#./tool/checkimg.exe bin/flashswp.img

#raw bin
#echo "create flash.txt"
#cp -a bin/flash.bin bin/flashswp.bin
#./tool/bin2txt bin/flash.txt bin/flashswp.bin
#./tool/programflash bin/MEM.TXT bin/flash.txt
echo "boot done"
