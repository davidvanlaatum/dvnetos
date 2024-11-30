#ifndef SERIAL_H
#define SERIAL_H
#include "cstdint"

namespace serial {
  namespace x86_64 {
    class Serial {
    public:
      explicit Serial(const uint16_t base) : base(base) {
      }

      void init(uint64_t hhdnOffset);

      void write(const char *text, size_t length);

    private:
      uint16_t base;
    };
  }

  using Serial = x86_64::Serial;

  extern Serial defaultSerial;
}


#endif //SERIAL_H
