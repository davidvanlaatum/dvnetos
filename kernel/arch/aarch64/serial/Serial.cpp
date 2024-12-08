#include "Serial.h"
#include <framebuffer/VirtualConsole.h>
#include <memutil.h>
#include "cstring"
#include "memory/paging.h"
#include "utils/panic.h"

using memory::addToPointer;

namespace serial {
  char serialBuffer[0x10000];
  Serial defaultSerial(reinterpret_cast<uint32_t *>(0x9000000), serialBuffer, sizeof(serialBuffer));
  constexpr auto FR = 0x18;
  constexpr auto IBRD = 0x24;
  constexpr auto FBRD = 0x28;
  constexpr auto LCRH = 0x2C;
  constexpr auto CR = 0x30;

  namespace aarch64 {
    void Serial::init(const uint64_t hhdmOffset) {
      memory::paging.mapMemory(getBaseAddr(), getBaseAddr() + hhdmOffset, PAGE_SIZE, 1, 0x700);
      base = addToPointer(base, hhdmOffset);
      if (const auto fr = *addToPointer(base, FR); fr != 0xFFFFFFFF && fr != 0x00000000) {
        kprint("Serial Port Found\n");

        *addToPointer(base, CR) = 0x0;

        *addToPointer(base, IBRD) = static_cast<uint32_t>(26);
        *addToPointer(base, FBRD) = static_cast<uint32_t>(3);
        *addToPointer(base, LCRH) = (1 << 4) | (3 << 5);

        *addToPointer(base, CR) = (1 << 0) | (1 << 8) | (1 << 9);

        enabled = true;
        write(buffer, bufferUsed);
        bufferUsed = 0;
      } else {
        memory::paging.unmapMemory(getBaseAddr(), 1, PAGE_SIZE);
        kprint("Serial not found\n");
      }
    }

    void Serial::write(const char *text, const size_t length) {
      if (enabled) {
        for (size_t i = 0; i < length; i++) {
          // Wait for UART to be ready to transmit
          while ((*addToPointer(base, FR) & 0x20) != 0) {
          }
          *base = text[i];
        }
      } else {
        kassert(bufferUsed + length <= bufferSize);
        memmove(buffer + bufferUsed, text, length);
        bufferUsed += length;
      }
    }
  } // namespace aarch64
} // namespace serial
