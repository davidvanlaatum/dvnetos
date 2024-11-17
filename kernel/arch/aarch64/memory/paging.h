#ifndef PAGING_H
#define PAGING_H
#include <cstdint>

struct limine_memmap_entry;

namespace memory {
    class Paging {
    public:
        void init(size_t count, limine_memmap_entry **mappings, uint64_t hhdmVirtualOffset,
                  uint64_t kernelVirtualOffset);
        // map a physical address to a virtual address that doesn't have to be page aligned
        void mapPartial(uint64_t physical_address, uint64_t virtual_address, size_t size, uint8_t flags,
                        uint8_t flags2) const;
    };

    extern Paging paging;
}
#endif //PAGING_H
