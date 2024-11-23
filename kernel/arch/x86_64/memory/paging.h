#ifndef PAGING_H
#define PAGING_H
#include <cstdint>
#include "utils/panic.h"

struct limine_memmap_entry;

namespace memory {
  class Paging {
  public:
    void init(size_t count, limine_memmap_entry **mappings, uint64_t hhdmVirtualOffset,
              uint64_t kernelVirtualOffset);

    // map a physical address to a virtual address that doesn't have to be page aligned
    void mapPartial(uint64_t physical_address, uint64_t virtual_address, size_t size, uint8_t flags,
                    uint8_t flags2) const;

    void mapMemory(uint64_t physical_address, uint64_t virtual_address, size_t size, uint8_t flags,
                   uint8_t flags2) const;

    [[nodiscard]] uint64_t makePageAligned(const uint64_t address) const {
      return address & ~PAGE_FLAGS_MASK & ~PAGE_FLAGS2_MASK;
    }

    static constexpr size_t PAGE_ENTRIES = PAGE_SIZE / sizeof(uint64_t);
    // Page table entry flags
    static constexpr uint64_t PAGE_PRESENT = 1 << 0;
    static constexpr uint64_t PAGE_WRITE = 1 << 1;
    static constexpr uint64_t PAGE_USER = 1 << 2;
    static constexpr uint64_t PAGE_WRITE_THROUGH = 1 << 3;
    static constexpr uint64_t PAGE_CACHE_DISABLE = 1 << 4;
    static constexpr uint64_t PAGE_ACCESSED = 1 << 5;
    static constexpr uint64_t PAGE_DIRTY = 1 << 6;
    static constexpr uint64_t PAGE_SIZE_FLAG = 1 << 7;
    static constexpr uint64_t PAGE_GLOBAL = 1 << 8;
    static constexpr uint64_t PAGE_FLAGS_MASK = 0xFFF;
    static constexpr uint64_t PAGE_NX = 1ULL << 63;
    static constexpr auto PAGE_FLAGS2_SHIFT = 52;
    // bits 52-63 are reserved for future use + NX
    static constexpr uint64_t PAGE_FLAGS2_MASK = 0xFFFll << PAGE_FLAGS2_SHIFT;

  protected:
    uint64_t *root = nullptr;

    struct PageTableRangeData {
      uint64_t virtualStart;
      uint64_t virtualEnd;
      uint64_t physicalStart;
      uint64_t physicalEnd;
      uint8_t flags;
      uint8_t flags2;
      uint64_t pageCount;
    };

    template<typename T>
    using pageTableToRangesCallback = void (*)(PageTableRangeData *, T *);

    template<typename T>
    using mapPhysicalToVirtual = void *(*)(uint64_t physical, T *data);

    template<typename T>
    static void pageTableToRanges(const uint64_t *table, mapPhysicalToVirtual<T> mapFunc,
                                  pageTableToRangesCallback<T> callback, T *data);

    template<typename T>
    using walkPageTableLeafCallback = bool (*)(uint64_t virtualAddress, uint64_t physicalAddress, uint8_t flags,
                                               uint8_t flags2, uint32_t pageSize, T *data);
    template<typename T>
    using walkPageTableLeafMissingCallback = bool (*)(uint64_t virtualAddress, T *data);

    template<typename T>
    static void walkPageTableLeaf(const uint64_t *table, mapPhysicalToVirtual<T> mapFunc,
                                  walkPageTableLeafCallback<T> leafCallback,
                                  walkPageTableLeafMissingCallback<T> missingCallback, T *data);

    static uint64_t pageIndexesToVirtual(const uint64_t l[], size_t count);

    struct virtualToPageIndexes_t {
      uint64_t l1;
      uint64_t l2;
      uint64_t l3;
      uint64_t l4;
    };

    static virtualToPageIndexes_t virtualToPageIndexes(uint64_t virtualAddress);

    static char *tableFlagsToString(uint8_t flags, uint8_t flags2);

    [[nodiscard]] uint64_t adjustPageTablePhysicalToVirtual(uint64_t in) const {
      return in + pageTableOffset;
    }

    [[nodiscard]] uint64_t adjustPageTableVirtualToPhysical(uint64_t in) const {
      return in - pageTableOffset;
    }

    uint64_t pageTableOffset = 0;
  };

  extern Paging paging;

  template<typename T>
  void Paging::walkPageTableLeaf(const uint64_t *table, const mapPhysicalToVirtual<T> mapFunc,
                                 const walkPageTableLeafCallback<T> leafCallback,
                                 const walkPageTableLeafMissingCallback<T> missingCallback, T *data) {
    auto done = false;
    for (uint64_t i = 0; i < PAGE_ENTRIES && !done; i++) {
      if (table[i] & PAGE_PRESENT) {
        const auto *l2Table = static_cast<uint64_t *>(mapFunc(table[i] & ~PAGE_FLAGS_MASK, data));
        for (uint64_t j = 0; j < PAGE_ENTRIES && !done; j++) {
          const uint64_t blockIdxL2[] = {i, j};
          if (l2Table[j] & PAGE_PRESENT) {
            if ((l2Table[j] & PAGE_SIZE_FLAG) != 0) {
              kpanic("unsupported 1GB page");
            }
            const auto *l3Table = static_cast<uint64_t *>(mapFunc(l2Table[j] & ~PAGE_FLAGS_MASK, data));
            for (uint64_t k = 0; k < PAGE_ENTRIES && !done; k++) {
              const uint64_t blockIdxL3[] = {i, j, k};
              if (l3Table[k] & PAGE_PRESENT) {
                if (l3Table[k] & PAGE_SIZE_FLAG) {
                  if (!leafCallback(pageIndexesToVirtual(blockIdxL3, 3),
                                    l3Table[k] & ~PAGE_FLAGS_MASK & ~PAGE_FLAGS2_MASK,
                                    l3Table[k] & PAGE_FLAGS_MASK & ~PAGE_SIZE_FLAG,
                                    (l3Table[k] & PAGE_FLAGS2_MASK) >> PAGE_FLAGS2_SHIFT, PAGE_SIZE * PAGE_ENTRIES,
                                    data)) {
                    done = true;
                  }
                } else {
                  const auto *l4Table = static_cast<uint64_t *>(mapFunc(l3Table[k] & ~PAGE_FLAGS_MASK, data));
                  for (uint64_t l = 0; l < PAGE_ENTRIES && !done; l++) {
                    const uint64_t blockIdxL4[] = {i, j, k, l};
                    if (l4Table[l] & PAGE_PRESENT) {
                      if (!leafCallback(pageIndexesToVirtual(blockIdxL4, 4),
                                        l4Table[l] & ~PAGE_FLAGS_MASK & ~PAGE_FLAGS2_MASK,
                                        l4Table[l] & PAGE_FLAGS_MASK & ~PAGE_SIZE_FLAG,
                                        (l4Table[l] & PAGE_FLAGS2_MASK) >> PAGE_FLAGS2_SHIFT, PAGE_SIZE, data)) {
                        done = true;
                      }
                    } else if (!missingCallback(pageIndexesToVirtual(blockIdxL4, 4), data)) {
                      done = true;
                    }
                  }
                }
              } else if (!missingCallback(pageIndexesToVirtual(blockIdxL3, 3), data)) {
                done = true;
              }
            }
          } else if (!missingCallback(pageIndexesToVirtual(blockIdxL2, 2), data)) {
            done = true;
          }
        }
      } else if (!missingCallback(pageIndexesToVirtual(&i, 1), data)) {
        done = true;
      }
    }
  }

  template<typename T>
  void Paging::pageTableToRanges(const uint64_t *table, const mapPhysicalToVirtual<T> mapFunc,
                                 const pageTableToRangesCallback<T> callback, T *data) {
    struct Data {
      PageTableRangeData currentBlock;
      bool inBlock;
      T *data;
      pageTableToRangesCallback<T> callback;
      mapPhysicalToVirtual<T> mapFunc;
    } lData = {
          .data = data,
          .callback = callback,
          .mapFunc = mapFunc,
        };

    walkPageTableLeafCallback<Data> leafCallback = [](uint64_t virtualAddress, uint64_t physicalAddress, uint8_t flags,
                                                      uint8_t flags2, uint32_t pageSize, Data *d) -> bool {
      flags &= ~(PAGE_ACCESSED | PAGE_DIRTY);
      if (d->inBlock) {
        if (d->currentBlock.physicalEnd + 1 == physicalAddress && d->currentBlock.flags == flags && d->currentBlock.
            flags2 == flags2) {
          d->currentBlock.physicalEnd = physicalAddress + pageSize - 1;
          d->currentBlock.virtualEnd = virtualAddress + pageSize - 1;
          d->currentBlock.pageCount += pageSize / PAGE_SIZE;
        } else {
          d->callback(&d->currentBlock, d->data);
          d->inBlock = false;
        }
      }
      if (!d->inBlock) {
        d->currentBlock.physicalStart = physicalAddress;
        d->currentBlock.physicalEnd = d->currentBlock.physicalStart + pageSize - 1;
        d->currentBlock.virtualStart = virtualAddress;
        d->currentBlock.virtualEnd = d->currentBlock.virtualStart + pageSize - 1;
        d->currentBlock.pageCount = pageSize / PAGE_SIZE;
        d->currentBlock.flags = flags;
        d->currentBlock.flags2 = flags2;
        d->inBlock = true;
      }
      return true;
    };
    walkPageTableLeafMissingCallback<Data> missingCallback = [](uint64_t, Data *d) -> bool {
      if (d->inBlock) {
        d->callback(&d->currentBlock, d->data);
        d->inBlock = false;
      }
      return true;
    };
    mapPhysicalToVirtual<Data> localMap = [](uint64_t physical, Data *d) -> void *{
      return reinterpret_cast<void *>(d->mapFunc(physical, d->data));
    };
    walkPageTableLeaf(table, localMap, leafCallback, missingCallback, &lData);
  }
}
#endif //PAGING_H
