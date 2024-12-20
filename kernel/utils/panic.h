#ifndef PANIC_H
#define PANIC_H

#include <cstdint>

#define kpanic(msg) panic(&__FILE__[sizeof(CMAKE_SOURCE_DIR)], __LINE__, msg)
#define kpanicf(msg, ...) panicf(&__FILE__[sizeof(CMAKE_SOURCE_DIR)], __LINE__, msg, __VA_ARGS__)
#define kassert(cond)                                                                                                  \
  if (!(cond))                                                                                                         \
  kpanic("assertion failed: " #cond)
#define kassertf(cond, msg, ...)                                                                                       \
  if (!(cond))                                                                                                         \
  panicf(&__FILE__[sizeof(CMAKE_SOURCE_DIR)], __LINE__, "%s " msg, "assertion failed: " #cond, __VA_ARGS__)

[[noreturn]] extern void halt();

[[noreturn]] extern void panic(const char *file, uint32_t line, const char *msg);

[[noreturn]] extern void panicf(const char *file, uint32_t line, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#endif // PANIC_H
