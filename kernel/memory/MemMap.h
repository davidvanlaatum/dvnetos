#ifndef MEMMAP_H
#define MEMMAP_H

#include <cstdint>

namespace memory {
  struct UsableMemory {
    uint64_t base;
    uint64_t length;
    uint8_t pageAllocated[1];
  };

  class MemMap {
  public:
    void init();

  protected:
    // UsableMemory *usableMemory[];
  };

  extern MemMap memMap;
}; // namespace memory


#endif // MEMMAP_H
