# 基于BootRom的Demo程序
## 概述  
	本程序集主要基于BootRom运行环境的Demo程序。提供给用户开发使用

## 编译器
* 编译Cortex-M4需要arm-none-eabi- 交叉编译器
* 编译RISC-V64需要riscv64-unknown-elf- 交叉编译器
** 按照编译器之后将环境变量增加到机器中

## 编译说明

### ARM 编译
1. 先在cortexm4_common路径下make，获取common库
2. 在cortexm4_demo根路径下执行make即可编译 make ENCPY=0

### RISC-V64 编译
1. 先在riscv64_common路径下make，获取common库
2. 在riscv64_demo根路径下执行make即可编译 make ENCPY=0

### 总的编译
在工程的根目录下有一个总的makefile文件，单独执行make可以同时编译Cortex-M4和RISC-V64程序。并生成flash镜像。当然你也可以单独编译

### flash镜像制作
执行./tool/bootgen.sh工具可以生成flash相关的bin文件，以及模拟测试flash.txt文件.注意：如果使用总编译，已经包含生成flash镜像功能。

## tool说明
	tool下面的工具，由BootRom的tool/src源码编译而来，如果BootRom的tool的工具有更新，重新获取源码并编译，放在此目录下的tool下。

