#ifndef MEMUTIL_H
#define MEMUTIL_H
#include <cstdint>

namespace memory {
  template<typename T>
  T *addToPointer(T *ptr, const uint64_t offset) {
    return reinterpret_cast<T *>(reinterpret_cast<uint64_t>(ptr) + offset);
  }

  template<typename T>
  T *subFromPointer(T *ptr, const uint64_t offset) {
    return reinterpret_cast<T *>(reinterpret_cast<uint64_t>(ptr) - offset);
  }

  template<typename T>
  void *toPtr(T ptr) {
    return reinterpret_cast<void *>(ptr);
  }
}
#endif //MEMUTIL_H
