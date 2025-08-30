/**
 * @file serial_utils.c
 * @brief 串口通信底层实现
 * @version 1.0
 * @date 2025-08-30
 */

#include "chip_detection.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ================================
 * 串口底层操作函数
 * ================================ */

/**
 * @brief 打开串口
 */
static int serial_open(const char *port, uint32_t baud_rate) {
    if (!port) return -1;
    
#ifdef _WIN32
    HANDLE h = CreateFile(port, GENERIC_READ | GENERIC_WRITE,
                         0, NULL, OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (h == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    
    if (!GetCommState(h, &dcb)) {
        CloseHandle(h);
        return -1;
    }
    
    dcb.BaudRate = baud_rate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fAbortOnError = FALSE;
    
    if (!SetCommState(h, &dcb)) {
        CloseHandle(h);
        return -1;
    }
    
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 0;
    
    SetCommTimeouts(h, &timeouts);
    
    return (int)(intptr_t)h;
    
#else // Linux/Unix
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }
    
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }
    
    // 设置波特率
    speed_t speed;
    switch (baud_rate) {
        case 9600:    speed = B9600; break;
        case 19200:   speed = B19200; break;
        case 38400:   speed = B38400; break;
        case 57600:   speed = B57600; break;
        case 115200:  speed = B115200; break;
        case 230400:  speed = B230400; break;
        case 460800:  speed = B460800; break;
        case 921600:  speed = B921600; break;
        default:      speed = B115200; break;
    }
    
    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);
    
    // 配置串口参数
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8位数据位
    tty.c_iflag &= ~IGNBRK;                         // 禁用忽略break
    tty.c_lflag = 0;                                // 原始模式
    tty.c_oflag = 0;                                // 原始输出
    tty.c_cflag |= (CLOCAL | CREAD);                // 启用接收器，忽略modem状态
    tty.c_cflag &= ~(PARENB | PARODD);              // 无奇偶校验
    tty.c_cflag &= ~CSTOPB;                         // 1位停止位
    tty.c_cflag &= ~CRTSCTS;                        // 禁用硬件流控
    
    // 设置超时
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }
    
    return fd;
#endif
}

/**
 * @brief 关闭串口
 */
static void serial_close(int fd) {
    if (fd < 0) return;
    
#ifdef _WIN32
    CloseHandle((HANDLE)(intptr_t)fd);
#else
    close(fd);
#endif
}

/**
 * @brief 写数据到串口
 */
static int serial_write_data(int fd, const char *data, int len) {
    if (fd < 0 || !data || len <= 0) return -1;
    
#ifdef _WIN32
    DWORD written;
    if (!WriteFile((HANDLE)(intptr_t)fd, data, len, &written, NULL)) {
        return -1;
    }
    return written;
#else
    return write(fd, data, len);
#endif
}

/**
 * @brief 从串口读数据 (带超时)
 */
static int serial_read_data(int fd, char *buffer, int size, int timeout_ms) {
    if (fd < 0 || !buffer || size <= 0) return -1;
    
    int total_read = 0;
    struct timeval start, now;
    gettimeofday(&start, NULL);
    
#ifdef _WIN32
    HANDLE h = (HANDLE)(intptr_t)fd;
    DWORD read_bytes;
    
    while (total_read < size - 1) {
        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - start.tv_sec) * 1000 + 
                      (now.tv_usec - start.tv_usec) / 1000;
        
        if (elapsed >= timeout_ms) break;
        
        if (ReadFile(h, buffer + total_read, 1, &read_bytes, NULL) && read_bytes > 0) {
            total_read += read_bytes;
        } else {
            Sleep(1);
        }
    }
#else
    while (total_read < size - 1) {
        gettimeofday(&now, NULL);
        long elapsed = (now.tv_sec - start.tv_sec) * 1000 + 
                      (now.tv_usec - start.tv_usec) / 1000;
        
        if (elapsed >= timeout_ms) break;
        
        int n = read(fd, buffer + total_read, size - total_read - 1);
        if (n > 0) {
            total_read += n;
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        } else {
            usleep(1000); // 1ms
        }
    }
#endif
    
    buffer[total_read] = '\0';
    return total_read;
}

/**
 * @brief 刷新串口缓冲区
 */
static void serial_flush(int fd) {
    if (fd < 0) return;
    
#ifdef _WIN32
    PurgeComm((HANDLE)(intptr_t)fd, PURGE_RXCLEAR | PURGE_TXCLEAR);
#else
    tcflush(fd, TCIOFLUSH);
#endif
}

/**
 * @brief 设置DTR和RTS信号 (ESP32进入下载模式用)
 */
static int serial_set_dtr_rts(int fd, bool dtr, bool rts) {
    if (fd < 0) return -1;
    
#ifdef _WIN32
    HANDLE h = (HANDLE)(intptr_t)fd;
    
    if (dtr) {
        EscapeCommFunction(h, SETDTR);
    } else {
        EscapeCommFunction(h, CLRDTR);
    }
    
    if (rts) {
        EscapeCommFunction(h, SETRTS);
    } else {
        EscapeCommFunction(h, CLRRTS);
    }
    
    return 0;
#else
    int status;
    if (ioctl(fd, TIOCMGET, &status) == -1) {
        return -1;
    }
    
    if (dtr) {
        status |= TIOCM_DTR;
    } else {
        status &= ~TIOCM_DTR;
    }
    
    if (rts) {
        status |= TIOCM_RTS;
    } else {
        status &= ~TIOCM_RTS;
    }
    
    return ioctl(fd, TIOCMSET, &status);
#endif
}
