#include "Serial.h"

namespace serial {
  Serial defaultSerial(0x3F8);

  namespace x86_64 {
    void Serial::init([[maybe_unused]] uint64_t hhdnOffset) {
      // noop
    }

    void Serial::write(const char *text, size_t length) {
      while (*text) {
        asm volatile ("outb %0, %1" : : "a"(*text), "Nd"(base));
        text++;
      }
    }
  }
}
