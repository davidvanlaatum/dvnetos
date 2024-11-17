#ifndef PANIC_H
#define PANIC_H

#include <cstdint>

#define kpanic(msg) panic(__FILE__,__LINE__,msg)

[[noreturn]] extern void halt();

[[noreturn]] extern void panic(const char *file, uint32_t line, const char *msg);

#endif //PANIC_H
