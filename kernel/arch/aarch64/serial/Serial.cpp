#include "Serial.h"
#include "cstring"
#include "memory/paging.h"
#include "utils/panic.h"

namespace serial {
  char serialBuffer[0x10000];
  Serial defaultSerial(reinterpret_cast<uint32_t *>(0x9000000), serialBuffer, sizeof(serialBuffer));
  constexpr auto FR = 0x18;
  constexpr auto IBRD = 0x24;
  constexpr auto FBRD = 0x28;
  constexpr auto LCRH = 0x2C;
  constexpr auto CR = 0x30;

  namespace aarch64 {
    template<typename T>
    T *adjustPointer(T *ptr, const uint64_t offset) {
      return reinterpret_cast<T *>(reinterpret_cast<uint64_t>(ptr) + offset);
    }

    void Serial::init(const uint64_t hhdmOffset) {
      memory::paging.mapMemory(getBaseAddr(), getBaseAddr() + hhdmOffset, PAGE_SIZE, 1, 0x700);
      base = adjustPointer(base, hhdmOffset);
      *adjustPointer(base, CR) = 0x0;

      *adjustPointer(base, IBRD) = static_cast<uint32_t>(26);
      *adjustPointer(base, FBRD) = static_cast<uint32_t>(3);
      *adjustPointer(base, LCRH) = (1 << 4) | (3 << 5);

      *adjustPointer(base, CR) = (1 << 0) | (1 << 8) | (1 << 9);

      enabled = true;
      write(buffer, bufferUsed);
      bufferUsed = 0;
    }

    void Serial::write(const char *text, const size_t length) {
      if (enabled) {
        for (size_t i = 0; i < length; i++) {
          // Wait for UART to be ready to transmit
          while ((*adjustPointer(base, FR) & 0x20) != 0) {
          }
          *base = text[i];
        }
      } else {
        kassert(bufferUsed + length <= bufferSize);
        memmove(buffer + bufferUsed, text, length);
        bufferUsed += length;
      }
    }
  }
}
