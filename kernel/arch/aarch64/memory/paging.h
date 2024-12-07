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
    void mapPartial(uint64_t physical_address, uint64_t virtual_address, size_t size, uint64_t flags);

    void mapMemory(uint64_t physical_address, uint64_t virtual_address, size_t pageSize, size_t num_pages,
                   uint64_t flags);

    [[nodiscard]] static uint64_t makePageAligned(const uint64_t address) {
      return address & ~0xFFFull;
    }

    static constexpr size_t PAGE_ENTRIES = PAGE_SIZE / sizeof(uint64_t);
    static constexpr uint64_t PAGE_VALID = 1 << 0;
    static constexpr uint64_t PAGE_TABLE = 1 << 1;
    static constexpr uint64_t PAGE_ADDR_MASK = 0x000FFFFFFFFFF000ull;
    static constexpr uint64_t PAGE_FLAGS_MASK = ~PAGE_ADDR_MASK;
    static constexpr uint64_t PAGE_ACCESSED = 0;
    static constexpr uint64_t PAGE_DIRTY = 0;

  protected:
    uint64_t *root1 = nullptr;
    uint64_t *root2 = nullptr;
    uint64_t tcr_el1 = 0;
    uint64_t pageTableOffset = 0;
    uint64_t higherHalfOffset = 0;

    struct PageTableRangeData {
      uint64_t virtualStart;
      uint64_t virtualEnd;
      uint64_t physicalStart;
      uint64_t physicalEnd;
      uint64_t flags;
      uint64_t pageCount;
      uint64_t pageSize;
    };

    template<typename T>
    using pageTableToRangesCallback = void (*)(PageTableRangeData *, T *);

    template<typename T>
    using mapPhysicalToVirtual = void *(*)(uint64_t physical, T *data);

    template<typename T>
    void pageTableToRanges(mapPhysicalToVirtual<T> mapFunc, pageTableToRangesCallback<T> callback, T *data);

    template<typename T>
    using walkPageTableLeafCallback = bool (*)(uint64_t virtualAddress, uint64_t physicalAddress, uint64_t flags,
                                               uint32_t pageSize, T *data);
    template<typename T>
    using walkPageTableLeafMissingCallback = bool (*)(uint64_t virtualAddress, T *data);

    template<typename T>
    void walkPageTableLeaf(mapPhysicalToVirtual<T> mapFunc,
                           walkPageTableLeafCallback<T> leafCallback,
                           walkPageTableLeafMissingCallback<T> missingCallback, T *data);

    template<typename T>
    bool walkPageTableLeaf(const uint64_t *table, bool higherHalf, mapPhysicalToVirtual<T> mapFunc,
                           walkPageTableLeafCallback<T> leafCallback,
                           walkPageTableLeafMissingCallback<T> missingCallback, T *data);

    uint64_t pageIndexesToVirtual(const uint64_t l[], size_t count, bool higherHalf) const;

    struct virtualToPageIndexes_t {
      bool higherHalf;
      uint64_t l1;
      uint64_t l2;
      uint64_t l3;
      uint64_t l4;
    };

    [[nodiscard]] virtualToPageIndexes_t virtualToPageIndexes(uint64_t virtualAddress) const;

    static char *tableFlagsToString(uint64_t flags);

    [[nodiscard]] uint64_t adjustPageTablePhysicalToVirtual(const uint64_t in) const {
      return in + pageTableOffset;
    }

    [[nodiscard]] uint64_t adjustPageTableVirtualToPhysical(const uint64_t in) const {
      return in - pageTableOffset;
    }

    static void setPageTableEntry(uint64_t *table, uint16_t index, uint8_t level, uint64_t virtualAddress,
                                  uint64_t physicalAddress, uint64_t flags);

    static void invalidateCache();
  };

  extern Paging paging;

  template<typename T>
  void Paging::walkPageTableLeaf(const mapPhysicalToVirtual<T> mapFunc, const walkPageTableLeafCallback<T> leafCallback,
                                 const walkPageTableLeafMissingCallback<T> missingCallback, T *data) {
    for (int i = 0; i < 2; i++) {
      const auto *table = i == 0 ? root1 : root2;
      if (table == nullptr) {
        continue;
      }
      if (walkPageTableLeaf(table, i == 1, mapFunc, leafCallback, missingCallback, data)) {
        return;
      }
    }
  }

  template<typename T>
  bool Paging::walkPageTableLeaf(const uint64_t *table, const bool higherHalf,
                                 const mapPhysicalToVirtual<T> mapFunc, const walkPageTableLeafCallback<T> leafCallback,
                                 const walkPageTableLeafMissingCallback<T> missingCallback, T *data) {
    for (uint64_t i = 0; i < PAGE_ENTRIES; i++) {
      auto l1Virtual = pageIndexesToVirtual(&i, 1, higherHalf);
      if (table[i] & PAGE_VALID) {
        if ((table[i] & PAGE_TABLE) == 0) {
          kpanic("unsupported huge page");
        }
        const auto *l2Table = static_cast<uint64_t *>(mapFunc(table[i] & PAGE_ADDR_MASK, data));
        for (uint64_t j = 0; j < PAGE_ENTRIES; j++) {
          const uint64_t blockIdxL2[] = {i, j};
          auto l2Virtual = pageIndexesToVirtual(blockIdxL2, 2, higherHalf);
          if (l2Table[j] & PAGE_VALID) {
            if ((l2Table[j] & PAGE_TABLE) == 0) {
              kpanic("unsupported 1GB page");
            }
            const auto *l3Table = static_cast<uint64_t *>(mapFunc(l2Table[j] & PAGE_ADDR_MASK, data));
            for (uint64_t k = 0; k < PAGE_ENTRIES; k++) {
              const uint64_t blockIdxL3[] = {i, j, k};
              auto l3Virtual = pageIndexesToVirtual(blockIdxL3, 3, higherHalf);
              if (l3Table[k] & PAGE_VALID) {
                if ((l3Table[k] & PAGE_TABLE) == 0) {
                  if (!leafCallback(l3Virtual, l3Table[k] & PAGE_ADDR_MASK,
                                    l3Table[k] & PAGE_FLAGS_MASK, PAGE_SIZE * PAGE_ENTRIES, data)) {
                    return true;
                  }
                } else {
                  const auto *l4Table = static_cast<uint64_t *>(mapFunc(l3Table[k] & PAGE_ADDR_MASK, data));
                  for (uint64_t l = 0; l < PAGE_ENTRIES; l++) {
                    const uint64_t blockIdxL4[] = {i, j, k, l};
                    const auto l4Virtual = pageIndexesToVirtual(blockIdxL4, 4, higherHalf);
                    if (l4Table[l] & PAGE_VALID) {
                      if (!leafCallback(l4Virtual, l4Table[l] & PAGE_ADDR_MASK, l4Table[l] & PAGE_FLAGS_MASK & ~PAGE_VALID & ~PAGE_TABLE,
                                        PAGE_SIZE, data)) {
                        return true;
                      }
                    } else if (!missingCallback(l4Virtual, data)) {
                      return true;
                    }
                  }
                }
              } else if (!missingCallback(l3Virtual, data)) {
                return true;
              }
            }
          } else if (!missingCallback(l2Virtual, data)) {
            return true;
          }
        }
      } else if (!missingCallback(l1Virtual, data)) {
        return true;
      }
    }
    return false;
  }


  template<typename T>
  void Paging::pageTableToRanges(const mapPhysicalToVirtual<T> mapFunc, const pageTableToRangesCallback<T> callback,
                                 T *data) {
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

    walkPageTableLeafCallback<Data> leafCallback = [](uint64_t virtualAddress, uint64_t physicalAddress, uint64_t flags,
                                                      uint32_t pageSize, Data *d) -> bool {
      // flags &= ~(PAGE_ACCESSED | PAGE_DIRTY);
      if (d->inBlock) {
        if (d->currentBlock.physicalEnd + 1 == physicalAddress && d->currentBlock.flags == flags && d->currentBlock.
            pageSize == pageSize) {
          d->currentBlock.physicalEnd = physicalAddress + pageSize - 1;
          d->currentBlock.virtualEnd = virtualAddress + pageSize - 1;
          ++d->currentBlock.pageCount;
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
        d->currentBlock.pageCount = 1;
        d->currentBlock.pageSize = pageSize;
        d->currentBlock.flags = flags;
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
    walkPageTableLeaf(localMap, leafCallback, missingCallback, &lData);
  }
}
#endif //PAGING_H
