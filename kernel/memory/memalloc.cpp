#include "memalloc.h"
#include "utils/debug.h"

struct MemBlock {
  size_t size;

  union {
    MemBlock *next;
    char data[1];
  };
};

static_assert(sizeof(MemBlock) == 16);

constexpr auto memBlockHeaderSize = sizeof(MemBlock) - sizeof(decltype(MemBlock::next));

void MemPool::removeFreeBlock(MemBlock *block, MemBlock *prev) {
  sizeFree -= block->size;
  if (prev == nullptr) {
    freeBlocks = block->next;
  } else {
    prev->next = block->next;
  }
  block->next = nullptr;
}

MemBlock *MemPool::findExistingFreeBlock(const size_t size) {
  auto ptr = this->freeBlocks;
  MemBlock *prev = nullptr;
  while (ptr) {
    if (ptr->size >= size) {
      DEBUG_PRINT("Found block of size " << ptr->size << " for " << size << " bytes in use: " << sizeInUse
                                         << " free: " << sizeFree);
      this->sizeInUse += ptr->size;
      removeFreeBlock(ptr, prev);
      DEBUG_PRINT("in use: " << this->sizeInUse << " free: " << this->sizeFree);
      return ptr;
    }
    prev = ptr;
    ptr = ptr->next;
  }
  return nullptr;
}

void *MemPool::alloc(const size_t size) {
  DEBUG_PRINT("Allocating " << size << " bytes");
  for (auto i = 0; i <= 1; i++) {
    auto block = findExistingFreeBlock(size);
    if (block != nullptr) {
      DEBUG_PRINT("Allocated " << size << " bytes from existing block of size " << block->size);
      if (block->size > size + memBlockHeaderSize) {
        const auto next = reinterpret_cast<MemBlock *>(block->data + size);
        next->size = block->size - size - memBlockHeaderSize;
        DEBUG_PRINT("Returning " << size << " bytes and adding " << block->size
                                 << " bytes back to free list. in use: " << sizeInUse << " free: " << sizeFree);
        sizeInUse -= next->size + memBlockHeaderSize;
        DEBUG_PRINT("in use: " << this->sizeInUse << " free: " << this->sizeFree);
        addFreeBlock(next);
        block->size = size;
      }
      allocCount++;
      return block->data;
    }
    const auto pages = (size + memBlockHeaderSize) / PAGE_SIZE + 1;
    DEBUG_PRINT("no pre-allocated block found, allocating " << pages << " pages");
    const auto page = getPage(pages);
    if (page == nullptr) {
      return nullptr;
    }
    block = static_cast<MemBlock *>(page);
    block->size = pages * PAGE_SIZE - memBlockHeaderSize;
    addFreeBlock(block);
  }
  return nullptr;
}

void MemPool::free(void *ptr) {
  if (ptr == nullptr) {
    return;
  }
  if (const auto block = reinterpret_cast<MemBlock *>(static_cast<char *>(ptr) - memBlockHeaderSize);
      block->size >= memBlockHeaderSize) {
    DEBUG_PRINT("Freeing " << block->size << " bytes");
    this->sizeInUse -= block->size;
    addFreeBlock(block);
    freeCount++;
  }
}

void MemPool::addFreeBlock(MemBlock *block) {
  DEBUG_PRINT("Adding " << block->size << " bytes to free list. in use: " << this->sizeInUse
                        << " free: " << this->sizeFree);
  bool merged;
  MemBlock **addAt = nullptr;
  do {
    merged = false;
    auto n = this->freeBlocks;
    MemBlock *prev = nullptr;
    addAt = &this->freeBlocks;
    while (n) {
      if (mergeIfPossible(&n, &block, prev)) {
        merged = true;
        continue;
      }
      if (!merged && block->size >= n->size) {
        addAt = &n->next;
      }
      prev = n;
      n = n->next;
    }
  } while (merged);
  block->next = *addAt;
  *addAt = block;
  this->sizeFree += block->size;
  DEBUG_PRINT("in use: " << this->sizeInUse << " free: " << this->sizeFree);
}

bool MemPool::mergeIfPossible(MemBlock **b1, MemBlock **b2, MemBlock *prev) {
  if (static_cast<void *>(reinterpret_cast<char *>(*b1) + (*b1)->size + memBlockHeaderSize) == *b2) {
    const auto next = (*b1)->next;
    removeFreeBlock(*b1, prev);
    (*b1)->size += (*b2)->size + memBlockHeaderSize;
    *b2 = (*b1);
    *b1 = next;
    return true;
  }
  if (static_cast<void *>(reinterpret_cast<char *>(*b2) + (*b2)->size + memBlockHeaderSize) == *b1) {
    const auto next = (*b1)->next;
    removeFreeBlock(*b1, prev);
    (*b2)->size += (*b1)->size + memBlockHeaderSize;
    *b1 = next;
    return true;
  }
  return false;
}

MemPool defaultPool;

void *kalloc(const size_t size) { return defaultPool.alloc(size); }

void kfree(void *ptr) { defaultPool.free(ptr); }
