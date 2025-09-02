#include <stdarg.h>

int __wrap_printf(const char *fmt, ...)
{
    (void)fmt;
    va_list ap;
    va_start(ap, fmt);
    va_end(ap);
    return 0;
}
