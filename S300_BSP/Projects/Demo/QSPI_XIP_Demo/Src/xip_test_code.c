/* XIP测试函数源代码 */
/* 这个文件用于生成将要写入Flash的机器码 */
/* 编译命令：arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -O2 -c xip_test_code.c -o xip_test_code.o */
/* 反汇编：arm-none-eabi-objdump -d xip_test_code.o */

/* 简单的加法函数 */
int __attribute__((section(".xip_code"))) xip_add_function(int a, int b)
{
    return a + b + 42;
}

/* 数组求和函数 */
int __attribute__((section(".xip_code"))) xip_array_sum(const int *array, int count)
{
    int sum = 0;
    for (int i = 0; i < count; i++)
    {
        sum += array[i];
    }
    return sum;
}

/* 斐波那契数列函数 */
int __attribute__((section(".xip_code"))) xip_fibonacci(int n)
{
    if (n <= 1) return n;
    int a = 0, b = 1, result = 0;
    for (int i = 2; i <= n; i++)
    {
        result = a + b;
        a = b;
        b = result;
    }
    return result;
}

/* 字符串长度函数 */
int __attribute__((section(".xip_code"))) xip_strlen(const char *str)
{
    int len = 0;
    while (str[len] != '\0')
    {
        len++;
    }
    return len;
}

/* 数据处理函数 */
typedef struct
{
    int x, y;
    int result;
} point_t;

void __attribute__((section(".xip_code"))) xip_process_points(point_t *points, int count)
{
    for (int i = 0; i < count; i++)
    {
        points[i].result = points[i].x * points[i].x + points[i].y * points[i].y;
    }
}
