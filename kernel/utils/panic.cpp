
#include "utils/panic.h"
#include "framebuffer/VirtualConsole.h"

void panic(const char *file, const uint32_t line, const char *msg) {
    framebuffer::defaultVirtualConsole.appendFormattedText("%s:%d: %s\n", file, line, msg);
    halt();
}

[[noreturn]] void halt() {
    for (;;) {
#if defined (__x86_64__)
        asm ("hlt");
#elif defined (__aarch64__) || defined (__riscv)
        asm ("wfi");
#elif defined (__loongarch64)
        asm ("idle 0");
#endif
    }
}
