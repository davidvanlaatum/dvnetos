#ifndef STRING_H
#define STRING_H
#include <stddef.h>
#include <stdint.h>

extern "C" {
size_t strlen(const char *str);
char *strncpy(char *dest, const char *src, size_t n);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
char *strncat(char *dest, const char *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
}

#endif // STRING_H
