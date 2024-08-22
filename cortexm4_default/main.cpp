/*** 
 * @Author       : xingxinonline
 * @Date         : 2024-08-22 14:51:12
 * @LastEditors  : xingxinonline
 * @LastEditTime : 2024-08-22 15:09:24
 * @FilePath     : \\ne004-plus\\cortexm4_default\\main.cpp
 * @Description  : 
 * @
 * @Copyright (c) 2024 by xinhao.pan@pimchip.cn, All Rights Reserved. 
 */

#include <cstdio>  // 用于使用 printf 函数

// 使用 extern "C" 来确保正确链接 C 函数
extern "C" {
    int hal_init(void);
}

int main() {
    // 调用外部的C函数（如有必要）
    // start_riscv(42);  // 示例调用，42 是传递的参数
    hal_init();
    // 输出 "Hello, World" 到标准输出（可能被重定向到 UART）
    printf("Hello, World!\n");

    while (1) {
        // 主循环，在嵌入式系统中通常是一个无限循环
    }

    return 0;
}
