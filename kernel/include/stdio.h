#ifndef STDIO_H
#define STDIO_H

#include <cstdarg>

int ksnprintf(char *buffer, size_t n, const char *format, ...) __attribute__((format(printf, 3, 4)));

int kvsnprintf(char *buffer, size_t n, const char *format, va_list args);

#endif //STDIO_H
