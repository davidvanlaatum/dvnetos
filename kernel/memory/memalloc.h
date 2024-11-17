#ifndef MEMALLOC_H
#define MEMALLOC_H

#include <cstddef>
#include "get-page.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif
struct MemBlock;

typedef void *(*getPage_t)(size_t);

typedef void (*freePage_t)(void *);

class MemPool {
public:
    MemPool() = default;

    [[nodiscard]] explicit MemPool(const getPage_t get_page, const freePage_t free_page) : getPage(get_page), freePage(free_page) {
    }

    [[nodiscard]] void *alloc(size_t size);

    void free(void *ptr);

    [[nodiscard]] size_t freeSize() const { return sizeFree; }
    [[nodiscard]] size_t usedSize() const { return sizeInUse; }
    [[nodiscard]] size_t countAlloc() const { return allocCount; }
    [[nodiscard]] size_t countFree() const { return freeCount; }

protected:
    [[nodiscard]] MemBlock *findExistingFreeBlock(size_t size);

    void addFreeBlock(MemBlock *block);

    bool mergeIfPossible(MemBlock **b1, MemBlock **b2, MemBlock *prev);
    void removeFreeBlock(MemBlock *block, MemBlock *prev);

    size_t sizeInUse = 0;
    size_t sizeFree = 0;
    size_t allocCount = 0;
    size_t freeCount = 0;
    size_t freeBlockCount = 0;
    MemBlock *freeBlocks = nullptr;

    getPage_t getPage = ::getPage;
    freePage_t freePage = ::freePage;
};

void *kalloc(size_t size);

void kfree(void *ptr);

#endif //MEMALLOC_H
