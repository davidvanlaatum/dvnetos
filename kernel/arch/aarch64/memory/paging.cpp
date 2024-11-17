#include "paging.h"

namespace memory {
    Paging paging;
    void Paging::init(size_t count, limine_memmap_entry **mappings, uint64_t hhdmVirtualOffset, uint64_t kernelVirtualOffset) {

    }

    void Paging::mapPartial(const uint64_t physical_address, const uint64_t virtual_address, const size_t size,
                        const uint8_t flags, const uint8_t flags2) const {
    }

}
