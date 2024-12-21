#include "utils/panic.h"
#include "alloca.h"
#include "cstdarg"
#include "framebuffer/VirtualConsole.h"
#include <cstdio>

void dumpStack(int drop) {
  void *frame_pointer;
#ifdef __x86_64__
  asm("mov %%rbp, %0" : "=r"(frame_pointer));
#elifdef __aarch64__
  asm("mov %0, x29" : "=r"(frame_pointer));
#else
#error "Unsupported architecture"
#endif
  while (frame_pointer) {
    auto frame = static_cast<void **>(frame_pointer);
    if (void *return_address = frame[1]; drop == 0 && return_address != nullptr) {
      kprintf("%p\n", return_address);
    } else {
      drop--;
    }
    frame_pointer = frame[0];
  }
}

void panic(const char *file, const uint32_t line, const char *msg) {
  kprintf("%s:%d: %s\n", file, line, msg);
  dumpStack(1);
  halt();
}

void panicf(const char *file, const uint32_t line, const char *fmt, ...) {
  va_list args;
  for (int i = 2;;) {
    va_start(args, fmt);
    const auto buf = static_cast<char *>(alloca(i));
    const int size = kvsnprintf(buf, i, fmt, args);
    va_end(args);
    if (size > i - 1) {
      i = size + 1;
      continue;
    }
    kprintf("%s:%d: %s\n", file, line, buf);
    break;
  }
  dumpStack(1);
  halt();
}

[[noreturn]] void halt() {
  for (;;) {
#if defined(__x86_64__)
    asm("hlt");
#elif defined(__aarch64__) || defined(__riscv)
    asm("wfi");
#elif defined(__loongarch64)
    asm("idle 0");
#endif
  }
}
