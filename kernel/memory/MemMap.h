#ifndef MEMMAP_H
#define MEMMAP_H

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
};


#endif //MEMMAP_H
