#ifndef SERIAL_H
#define SERIAL_H

#include <cstdint>

namespace serial {
  namespace aarch64 {
    class Serial {
    public:
      explicit Serial(volatile uint32_t *base, char *buffer, const size_t size) : base(base), buffer(buffer),
        bufferSize(size) {
      }

      void init(uint64_t hhdnOffset);

      void write(const char *text, size_t length);

    private:
      volatile uint32_t *base;
      char *buffer;
      size_t bufferSize;
      size_t bufferUsed = 0;
      bool enabled = false;

      [[nodiscard]] uint64_t getBaseAddr() const { return reinterpret_cast<uint64_t>(base); }
    };
  }

  using Serial = aarch64::Serial;

  extern Serial defaultSerial;
} // serial

#endif //SERIAL_H
