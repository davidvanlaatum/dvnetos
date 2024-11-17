#ifndef STDIO_H
#define STDIO_H

#include <cstdarg>
#include <stddef.h>

extern "C" {
    int ksnprintf(char *buffer, size_t n, const char *format, ...);
    int kvsnprintf(char *buffer, size_t n, const char *format, va_list args);
}

#endif //STDIO_H
