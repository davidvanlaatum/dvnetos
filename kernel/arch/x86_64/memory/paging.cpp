#include <cstring>
#include "paging.h"

#include "limine.h"
#include "framebuffer/VirtualConsole.h"
#include "utils/bytes.h"
#include "utils/panic.h"

namespace memory {
  __attribute__((used, section(".limine_requests")))
  volatile limine_paging_mode_request paging_request = {
    .id = LIMINE_PAGING_MODE_REQUEST,
    .revision = 0,
    .response = nullptr,
    .mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .max_mode = LIMINE_PAGING_MODE_X86_64_4LVL,
    .min_mode = LIMINE_PAGING_MODE_X86_64_4LVL,
  };

  constexpr size_t INITIAL_POOL_SIZE = 128;

  Paging paging;

  char initialPool[INITIAL_POOL_SIZE][PAGE_SIZE] __attribute__((aligned(PAGE_SIZE)));
  uint8_t initialPoolUsedCount = 0;

  void *initialPoolGetPage(size_t);

  void *(*GetPagePtr)(size_t count) = initialPoolGetPage;

  void *initialPoolGetPage(const size_t count) {
    const auto rt = reinterpret_cast<void *>(initialPool[initialPoolUsedCount]);
    if (initialPoolUsedCount + count >= INITIAL_POOL_SIZE) {
      kpanic("out of initial pool");
    }
    initialPoolUsedCount += count;
    return rt;
  }

  uint64_t Paging::pageIndexesToVirtual(const uint64_t l[], const size_t count) {
    uint64_t address = 0;
    if (count > 0) {
      address |= l[0] << 39;
      if (count > 1) {
        address |= l[1] << 30;
        if (count > 2) {
          address |= l[2] << 21;
          if (count > 3) {
            address |= l[3] << 12;
          }
        }
      }
    }
    // Sign-extend the address if the 48th bit is set
    if (address & (1ULL << 47)) {
      address |= 0xFFFF000000000000;
    }
    return address;
  }

  Paging::virtualToPageIndexes_t Paging::virtualToPageIndexes(const uint64_t virtualAddress) {
    return {
      (virtualAddress >> 39) & 0x1FF,
      (virtualAddress >> 30) & 0x1FF,
      (virtualAddress >> 21) & 0x1FF,
      (virtualAddress >> 12) & 0x1FF
    };
  }

  char *Paging::tableFlagsToString(const uint8_t flags, const uint8_t flags2) {
    static char buf[32];
    size_t n = 0;
    if (flags & PAGE_PRESENT) {
      buf[n++] = 'P';
    }
    if (flags & PAGE_WRITE) {
      buf[n++] = 'W';
    }
    if (flags & PAGE_USER) {
      buf[n++] = 'U';
    }
    if (flags & PAGE_WRITE_THROUGH) {
      buf[n++] = 'T';
    }
    if (flags & PAGE_CACHE_DISABLE) {
      buf[n++] = 'C';
    }
    if (flags & PAGE_SIZE_FLAG) {
      buf[n++] = 'S';
    }
    if (flags2 & PAGE_NX >> PAGE_FLAGS2_SHIFT) {
      buf[n++] = 'N';
      buf[n++] = 'X';
    }
    buf[n] = 0;
    return buf;
  };

  void Paging::init(const size_t count, limine_memmap_entry **mappings, const uint64_t hhdmVirtualOffset,
                    const uint64_t kernelVirtualOffset) {
    if (hhdmVirtualOffset % PAGE_SIZE != 0) {
      kpanic("virtual offset must be page aligned");
    }
    if (paging_request.response == nullptr) {
      kpanic("paging request not set");
    }
    if (paging_request.response->mode != LIMINE_PAGING_MODE_X86_64_4LVL) {
      kpanic("paging mode not supported");
    }
    uint64_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    framebuffer::defaultVirtualConsole.appendFormattedText("current cr3: %p/%p initial pool %p\n", cr3,
                                                           cr3 + hhdmVirtualOffset, initialPool);

    root = static_cast<uint64_t *>(GetPagePtr(1));
    pageTableOffset = kernelVirtualOffset;
    memset(root, 0, PAGE_SIZE);

    for (int i = 0; i < count; i++) {
      if (mappings[i]->type == LIMINE_MEMMAP_FRAMEBUFFER) {
        mapMemory(mappings[i]->base, framebuffer::defaultFramebuffer.getFramebufferVirtualAddress(),
                  mappings[i]->length, PAGE_PRESENT | PAGE_WRITE | PAGE_WRITE_THROUGH, 0);
        break;
      }
    }
    struct callbackData {
      size_t mapCount;
      limine_memmap_entry **mappings;
      Paging *paging;
      uint64_t hhdmVirtualOffset;
    };
    callbackData data = {count, mappings, this, hhdmVirtualOffset};
    pageTableToRangesCallback<callbackData> callback = [
        ](PageTableRangeData *page_table_range_data, callbackData *data) {
      static auto isTypeToMap = [](const uint64_t type) {
        return type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
               || type == LIMINE_MEMMAP_KERNEL_AND_MODULES
               || type == LIMINE_MEMMAP_FRAMEBUFFER;
      };
      static auto rangesOverlap = [](PageTableRangeData *page_table_range_data,
                                     limine_memmap_entry *entry) {
        if (page_table_range_data->physicalStart <= entry->base + entry->length &&
            page_table_range_data->physicalEnd >= entry->base) {
          return true;
        } else if (page_table_range_data->physicalStart <= entry->base && page_table_range_data->
                   physicalEnd >= entry->base) {
          return true;
        } else if (page_table_range_data->physicalStart <= entry->base + entry->length &&
                   page_table_range_data->physicalEnd >= entry->base + entry->length) {
          return true;
        } else {
          return false;
        }
      };
      bool mapped = false;
      for (size_t i = 0; i < data->mapCount; i++) {
        if (rangesOverlap(page_table_range_data, data->mappings[i])) {
          if (isTypeToMap(data->mappings[i]->type)) {
            char buff[32];
            framebuffer::defaultVirtualConsole.appendFormattedText(
              "block %p-%p/%p-%p %d(%s) %s %d\n", page_table_range_data->virtualStart,
              page_table_range_data->virtualEnd, page_table_range_data->physicalStart,
              page_table_range_data->physicalEnd, page_table_range_data->pageCount,
              bytesToHumanReadable(buff, sizeof(buff), page_table_range_data->pageCount * PAGE_SIZE),
              tableFlagsToString(page_table_range_data->flags, page_table_range_data->flags2),
              data->mappings[i]->type);
            data->paging->mapMemory(page_table_range_data->physicalStart,
                                    page_table_range_data->virtualStart,
                                    page_table_range_data->pageCount * PAGE_SIZE,
                                    page_table_range_data->flags, page_table_range_data->flags2);
            mapped = true;
            break;
          }
        }
      }
      if (!mapped && page_table_range_data->pageCount > 2) {
        char buff[32];
        framebuffer::defaultVirtualConsole.appendFormattedText(
          "not mapping %p-%p/%p-%p %d(%s) %s\n", page_table_range_data->virtualStart,
          page_table_range_data->virtualEnd, page_table_range_data->physicalStart,
          page_table_range_data->physicalEnd, page_table_range_data->pageCount,
          bytesToHumanReadable(buff, sizeof(buff), page_table_range_data->pageCount * PAGE_SIZE),
          tableFlagsToString(page_table_range_data->flags, page_table_range_data->flags2));
      }
    };
    mapPhysicalToVirtual<callbackData> mapper = [](const uint64_t v, callbackData *d) {
      return reinterpret_cast<void *>(v + d->hhdmVirtualOffset);
    };
    pageTableToRanges(reinterpret_cast<uint64_t *>(cr3 + hhdmVirtualOffset), mapper, callback, &data);

    // uint64_t currentBlockPhysicalStart = 0;
    // uint64_t currentBlockPhysicalEnd = 0;
    // uint64_t currentBlockVirtualStart = 0;
    // uint64_t currentBlockVirtualEnd = 0;
    // uint64_t currentBlockPageCount = 0;
    // bool inBlock = false;
    // size_t currentCount = 1;
    // auto currentBlockOverlapsMappings = [&] {
    //   for (int i = 0; i < count; i++) {
    //     if (currentBlockPhysicalStart >= mappings[i].physicalAddress
    //         && currentBlockPhysicalStart <= mappings[i].physicalAddress + mappings[i].size) {
    //       return i;
    //     }
    //     if (currentBlockPhysicalEnd >= mappings[i].physicalAddress
    //         && currentBlockPhysicalEnd <= mappings[i].physicalAddress + mappings[i].size) {
    //       return i;
    //     }
    //     if (currentBlockPhysicalStart <= mappings[i].physicalAddress
    //         && currentBlockPhysicalEnd >= mappings[i].physicalAddress + mappings[i].size) {
    //       return i;
    //     }
    //   }
    //   return -1;
    // };
    // auto endBlock = [&] {
    //   auto x = currentBlockOverlapsMappings();
    //   if (/*currentBlockPageCount > 1 || */x >= 0) {
    //     char buf[32];
    //     framebuffer::defaultVirtualConsole.appendFormattedText(
    //       "block %p-%p/%p-%p %d(%s) %d\n", currentBlockVirtualStart, currentBlockVirtualEnd,
    //       currentBlockPhysicalStart, currentBlockPhysicalEnd, currentBlockPageCount,
    //       bytesToHumanReadable(buf, sizeof(buf), currentBlockPageCount * PAGE_SIZE), x);
    //   }
    //   inBlock = false;
    // };
    // auto *currentRoot = reinterpret_cast<uint64_t *>(cr3 + hhdmVirtualOffset);
    // for (uint64_t i = 0; i < PAGE_ENTRIES; i++) {
    //   if (currentRoot[i] & PAGE_PRESENT != 0) {
    //     currentCount++;
    //     if (i == 511) {
    //       framebuffer::defaultVirtualConsole.appendFormattedText("current root entry %d is %p\n", i,
    //                                                              currentRoot[i]);
    //     }
    //     auto *currentL2 = reinterpret_cast<uint64_t *>((currentRoot[i] & ~PAGE_FLAGS_MASK) + hhdmVirtualOffset);
    //     for (uint64_t j = 0; j < PAGE_ENTRIES; j++) {
    //       if (currentL2[j] & PAGE_PRESENT != 0) {
    //         currentCount++;
    //         if (i == 511 && j == 510) {
    //           framebuffer::defaultVirtualConsole.appendFormattedText(
    //             "current l2 entry %d.%d is %p\n", i, j, currentL2[j]);
    //         }
    //         auto *currentL3 = reinterpret_cast<uint64_t *>(
    //           (currentL2[j] & ~PAGE_FLAGS_MASK) + hhdmVirtualOffset);
    //         for (uint64_t k = 0; k < PAGE_ENTRIES; k++) {
    //           if (currentL3[k] & PAGE_PRESENT != 0) {
    //             currentCount++;
    //             if (i == 511 && j == 510 && k == 0) {
    //               framebuffer::defaultVirtualConsole.appendFormattedText(
    //                 "current l3 entry %d.%d.%d is %p\n", i, j, k,
    //                 currentL3[k]);
    //             }
    //             auto *currentL4 = reinterpret_cast<uint64_t *>(
    //               (currentL3[k] & ~PAGE_FLAGS_MASK) + hhdmVirtualOffset);
    //             for (uint64_t l = 0; l < PAGE_ENTRIES; l++) {
    //               if (currentL4[l] & PAGE_PRESENT != 0) {
    //                 auto currentBlockPhysical = currentL4[l] & ~PAGE_FLAGS_MASK;
    //                 auto currentBlockVirtual = pageIndexesToVirtual(i, j, k, l);
    //                 if (i == 511 && j == 510 && k == 0 && l == 0) {
    //                   framebuffer::defaultVirtualConsole.appendFormattedText(
    //                     "current l4 entry %d.%d.%d.%d is %p/%p\n",
    //                     i, j, k, l, currentL4[l], currentBlockVirtual);
    //                 }
    //                 if (inBlock) {
    //                   if (currentBlockPhysicalEnd + 1 == currentBlockPhysical) {
    //                     currentBlockPhysicalEnd = currentBlockPhysical + PAGE_SIZE - 1;
    //                     currentBlockVirtualEnd = currentBlockVirtual + PAGE_SIZE - 1;
    //                     currentBlockPageCount++;
    //                   } else {
    //                     endBlock();
    //                   }
    //                 }
    //                 if (!inBlock && currentBlockPhysical != 0xfffffffffffff000) {
    //                   currentBlockPhysicalStart = currentBlockPhysical;
    //                   currentBlockPhysicalEnd = currentBlockPhysicalStart + PAGE_SIZE - 1;
    //                   currentBlockVirtualStart = currentBlockVirtual;
    //                   currentBlockVirtualEnd = currentBlockVirtualStart + PAGE_SIZE - 1;
    //                   currentBlockPageCount = 1;
    //                   inBlock = true;
    //                 }
    //               } else if (inBlock) {
    //                 endBlock();
    //               }
    //             }
    //           } else if (inBlock) {
    //             endBlock();
    //           }
    //         }
    //       } else if (inBlock) {
    //         endBlock();
    //       }
    //     }
    //   } else if (inBlock) {
    //     endBlock();
    //   }
    // }
    //
    // if (inBlock) {
    //   endBlock();
    // }
    //
    // auto flagsToStr = [](const uint64_t flags) {
    //   static char buf[32];
    //   size_t n = 0;
    //   if (flags & PAGE_PRESENT) {
    //     buf[n++] = 'P';
    //   }
    //   if (flags & PAGE_WRITE) {
    //     buf[n++] = 'W';
    //   }
    //   if (flags & PAGE_USER) {
    //     buf[n++] = 'U';
    //   }
    //   if (flags & PAGE_WRITE_THROUGH) {
    //     buf[n++] = 'T';
    //   }
    //   if (flags & PAGE_CACHE_DISABLE) {
    //     buf[n++] = 'C';
    //   }
    //   if (flags & PAGE_SIZE_FLAG) {
    //     buf[n++] = 'S';
    //   }
    //   buf[n] = 0;
    //   return buf;
    // };
    //
    // uint64_t lookupAddress = 0xffffffff80000000;
    // auto t = virtualToPageIndexes(lookupAddress);
    // auto x = pageIndexesToVirtual(t.l1, t.l2, t.l3, t.l4);
    // framebuffer::defaultVirtualConsole.appendFormattedText("lookup address %p is at %d.%d.%d.%d %p %d\n",
    //                                                        lookupAddress,
    //                                                        t.l1, t.l2, t.l3, t.l4, x, lookupAddress == x);
    //
    // uint64_t l1Index = (lookupAddress >> 39) & 0x1FF;
    // uint64_t l2Index = (lookupAddress >> 30) & 0x1FF;
    // uint64_t l3Index = (lookupAddress >> 21) & 0x1FF;
    // uint64_t l4Index = (lookupAddress >> 12) & 0x1FF;
    // framebuffer::defaultVirtualConsole.appendFormattedText("lookup address %p is at %d.%d.%d.%d\n", lookupAddress,
    //                                                        l1Index, l2Index, l3Index, l4Index);
    //
    // auto l1addr = currentRoot[l1Index];
    // if (l1addr & PAGE_PRESENT != 0) {
    //   auto *l2 = reinterpret_cast<uint64_t *>((l1addr & ~PAGE_FLAGS_MASK) + hhdmVirtualOffset);
    //   framebuffer::defaultVirtualConsole.appendFormattedText("l1 entry is %p/%p %s\n", l1addr, l2,
    //                                                          flagsToStr(l1addr & PAGE_FLAGS_MASK));
    //   auto l2addr = l2[l2Index];
    //   if (l2addr & PAGE_PRESENT != 0) {
    //     auto *l3 = reinterpret_cast<uint64_t *>((l2addr & ~PAGE_FLAGS_MASK) + hhdmVirtualOffset);
    //     framebuffer::defaultVirtualConsole.appendFormattedText("l2 entry is %p/%p %s\n", l2addr, l3,
    //                                                            flagsToStr(l2addr & PAGE_FLAGS_MASK));
    //     auto l3addr = l3[l3Index];
    //     if (l3addr & PAGE_PRESENT != 0) {
    //       auto *l4 = reinterpret_cast<uint64_t *>((l3addr & ~PAGE_FLAGS_MASK) + hhdmVirtualOffset);
    //       framebuffer::defaultVirtualConsole.appendFormattedText("l3 entry is %p/%p %s\n", l3addr, l4,
    //                                                              flagsToStr(l3addr & PAGE_FLAGS_MASK));
    //       auto l4addr = l4[l4Index];
    //       if (l4addr & PAGE_PRESENT != 0) {
    //         framebuffer::defaultVirtualConsole.appendFormattedText("l4 entry is %p %s\n", l4addr,
    //                                                                flagsToStr(l4addr & PAGE_FLAGS_MASK));
    //       }
    //     }
    //   }
    // }
    //
    //
    // framebuffer::defaultVirtualConsole.appendFormattedText("current paging table has %d tables\n", currentCount);
    //
    // root = static_cast<uint64_t *>(GetPagePtr(1));
    // memset(root, 0, PAGE_SIZE);
    // for (size_t i = 0; i < count; i++) {
    //   mapMemory(mappings[i].physicalAddress, mappings[i].virtualAddress, mappings[i].size);
    // }
    // applyOffset(kernelVirtualOffset);
    uint64_t rootPhysicalAddress = reinterpret_cast<uint64_t>(root) - kernelVirtualOffset;
    framebuffer::defaultVirtualConsole.appendFormattedText(
      "new paging table created at %p/%p using %d temporary tables\n", root,
      rootPhysicalAddress, initialPoolUsedCount);
    asm volatile("mov %0, %%cr3" : : "r"(rootPhysicalAddress) : "memory");
    framebuffer::defaultVirtualConsole.appendText("paging enabled\n");
  }

  /*
    template<typename T>
    void Paging::pageTableToRanges(const uint64_t *table, const uint64_t virtualOffset,
                                   const pageTableToRangesCallback<T> callback, T *data) {
      PageTableRangeData currentBlock{};
      bool inBlock = false;
      auto startOrContinueBlock = [&](uint64_t blockPhysical, uint64_t blockVirtual, uint8_t flags, uint64_t blockSize) {
        if (inBlock) {
          if (currentBlock.physicalEnd + 1 == blockPhysical
              && currentBlock.flags == flags) {
            currentBlock.physicalEnd = blockPhysical + blockSize - 1;
            currentBlock.virtualEnd = blockVirtual + blockSize - 1;
            currentBlock.pageCount += blockSize / PAGE_SIZE;
          } else {
            callback(&currentBlock, data);
            inBlock = false;
          }
        }
        if (!inBlock) {
          currentBlock.physicalStart = blockPhysical;
          currentBlock.physicalEnd = currentBlock.physicalStart + blockSize - 1;
          currentBlock.virtualStart = blockVirtual;
          currentBlock.virtualEnd = currentBlock.virtualStart + blockSize - 1;
          currentBlock.pageCount = blockSize / PAGE_SIZE;
          currentBlock.flags = flags;
          inBlock = true;
        }
      };
      for (uint64_t i = 0; i < PAGE_ENTRIES; i++) {
        if ((table[i] & PAGE_PRESENT) != 0) {
          const auto *l2Table = reinterpret_cast<uint64_t *>((table[i] & ~PAGE_FLAGS_MASK) + virtualOffset);
          for (uint64_t j = 0; j < PAGE_ENTRIES; j++) {
            if (l2Table[j] & PAGE_PRESENT) {
              if ((l2Table[j] & PAGE_SIZE_FLAG) != 0) {
                kpanic("unsupported 1GB page");
              }
              const auto *l3Table = reinterpret_cast<uint64_t *>((l2Table[j] & ~PAGE_FLAGS_MASK) + virtualOffset);
              for (uint64_t k = 0; k < PAGE_ENTRIES; k++) {
                if ((l3Table[k] & PAGE_PRESENT) != 0) {
                  if (l3Table[k] & PAGE_SIZE_FLAG) {
                    const uint64_t blockIdx[] = {i, j, k};
                    startOrContinueBlock(l3Table[k] & ~PAGE_FLAGS_MASK, pageIndexesToVirtual(blockIdx, 3),
                                         l3Table[k] & PAGE_FLAGS_MASK & ~PAGE_SIZE_FLAG, PAGE_SIZE * PAGE_ENTRIES);
                  } else {
                    const auto *l4Table = reinterpret_cast<uint64_t *>((l3Table[k] & ~PAGE_FLAGS_MASK) + virtualOffset);
                    for (uint64_t l = 0; l < PAGE_ENTRIES; l++) {
                      if ((l4Table[l] & PAGE_PRESENT) != 0) {
                        const uint64_t blockIdx[] = {i, j, k, l};
                        startOrContinueBlock(l4Table[l] & ~PAGE_FLAGS_MASK, pageIndexesToVirtual(blockIdx, 4),
                                             l4Table[l] & PAGE_FLAGS_MASK & ~PAGE_SIZE_FLAG, PAGE_SIZE);
                      } else if (inBlock) {
                        callback(&currentBlock, data);
                        inBlock = false;
                      }
                    }
                  }
                } else if (inBlock) {
                  callback(&currentBlock, data);
                  inBlock = false;
                }
              }
            } else if (inBlock) {
              callback(&currentBlock, data);
              inBlock = false;
            }
          }
        } else if (inBlock) {
          callback(&currentBlock, data);
          inBlock = false;
        }
      }

      if (inBlock) {
        callback(&currentBlock, data);
      }
    }*/

  void Paging::applyOffset(const uint64_t virtualOffset) const {
    framebuffer::defaultVirtualConsole.appendFormattedText("applying offset %p\n", virtualOffset);
    for (size_t l1 = 0; l1 < PAGE_ENTRIES; l1++) {
      if (root[l1] & PAGE_PRESENT) {
        const auto l2ptr = reinterpret_cast<uint64_t *>(root[l1] & ~PAGE_FLAGS_MASK);
        for (size_t l2 = 0; l2 < PAGE_ENTRIES; l2++) {
          if (l2ptr[l2] & PAGE_PRESENT) {
            const auto l3ptr = reinterpret_cast<uint64_t *>(l2ptr[l2] & ~PAGE_FLAGS_MASK);
            for (size_t l3 = 0; l3 < PAGE_ENTRIES; l3++) {
              if (l3ptr[l3] & PAGE_PRESENT) {
                const auto x = ((l3ptr[l3] - virtualOffset) & ~PAGE_FLAGS_MASK) | (
                                 l3ptr[l3] & PAGE_FLAGS_MASK);
                framebuffer::defaultVirtualConsole.appendFormattedText(
                  "updating page table entry from %p to %p\n", l3ptr[l3], x);
                l3ptr[l3] = x;
              }
            }
            const auto x = ((l2ptr[l2] - virtualOffset) & ~PAGE_FLAGS_MASK) | (l2ptr[l2] & PAGE_FLAGS_MASK);
            framebuffer::defaultVirtualConsole.appendFormattedText(
              "updating page table entry from %p to %p\n", l2ptr[l2], x);
            l2ptr[l2] = x;
          }
        }
        auto x = ((root[l1] - virtualOffset) & ~PAGE_FLAGS_MASK) | (root[l1] & PAGE_FLAGS_MASK);
        framebuffer::defaultVirtualConsole.appendFormattedText(
          "updating page table entry from %p to %p\n", root[l1], x);
        root[l1] = x;
      }
    }
  }

  void Paging::mapPartial(const uint64_t physical_address, const uint64_t virtual_address, const size_t size,
                          const uint8_t flags, const uint8_t flags2) const {
    const auto new_physical_address = makePageAligned(physical_address);
    const auto new_virtual_address = makePageAligned(virtual_address);
    auto new_size = size + PAGE_SIZE - (size % PAGE_SIZE);
    if (physical_address + size > new_physical_address + new_size) {
      new_size += PAGE_SIZE;
    }
    mapMemory(new_physical_address, new_virtual_address, new_size, flags, flags2);
  }

  void Paging::mapMemory(uint64_t physical_address, uint64_t virtual_address, const size_t size,
                         const uint8_t flags, const uint8_t flags2) const {
    const size_t num_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    if (num_pages * PAGE_SIZE != size) {
      kpanic("?\n");
    }
    framebuffer::defaultVirtualConsole.appendFormattedText(
      "mapping %p-%p to %p-%p (%d) %s\n", virtual_address, virtual_address + (num_pages * PAGE_SIZE) - 1,
      physical_address, physical_address + (num_pages * PAGE_SIZE) - 1, num_pages, tableFlagsToString(flags, flags2));
    if (physical_address % PAGE_SIZE != 0) {
      kpanic("physical address must be page aligned");
    }
    if (virtual_address % PAGE_SIZE != 0) {
      framebuffer::defaultVirtualConsole.appendFormattedText("virtual address %p is not page aligned %d\n",
                                                             virtual_address, virtual_address % PAGE_SIZE);
      kpanic("virtual address must be page aligned");
    }
    if (size % PAGE_SIZE != 0) {
      kpanic("size must be page aligned");
    }

    for (size_t i = 0; i < num_pages; ++i) {
      const auto idx = virtualToPageIndexes(virtual_address);

      if (!(root[idx.l1] & PAGE_PRESENT)) {
        auto *new_pdpt = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pdpt, 0, PAGE_SIZE);
        root[idx.l1] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pdpt)) | PAGE_PRESENT |
                       PAGE_WRITE | PAGE_USER;
      }
      const auto pdpt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(root[idx.l1] & ~PAGE_FLAGS_MASK));

      if (!(pdpt[idx.l2] & PAGE_PRESENT)) {
        auto *new_pd = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pd, 0, PAGE_SIZE);
        pdpt[idx.l2] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pd)) | PAGE_PRESENT | PAGE_WRITE
                       | PAGE_USER;
      }
      const auto pd = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pdpt[idx.l2] & ~PAGE_FLAGS_MASK));

      if (!(pd[idx.l3] & PAGE_PRESENT)) {
        if (virtual_address % (PAGE_SIZE * PAGE_ENTRIES) == 0
            && physical_address % (PAGE_SIZE * PAGE_ENTRIES) == 0
            && num_pages - i >= PAGE_ENTRIES) {
          framebuffer::defaultVirtualConsole.appendFormattedText(
            "mapping huge page at %p-%p/%p-%p\n", virtual_address, virtual_address + (PAGE_SIZE * PAGE_ENTRIES) - 1,
            physical_address, physical_address + (PAGE_SIZE * PAGE_ENTRIES) - 1);
          pd[idx.l3] = physical_address | PAGE_PRESENT | PAGE_SIZE_FLAG | flags
                       | static_cast<uint64_t>(flags2) << PAGE_FLAGS2_SHIFT;
          virtual_address += PAGE_SIZE * PAGE_ENTRIES;
          physical_address += PAGE_SIZE * PAGE_ENTRIES;
          i += PAGE_ENTRIES - 1;
          continue;
        }
        auto *new_pt = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pt, 0, PAGE_SIZE);
        pd[idx.l3] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pt)) | PAGE_PRESENT | PAGE_WRITE |
                     PAGE_USER;
      } else if (pd[idx.l3] & PAGE_SIZE_FLAG) {
        continue;
      }
      const auto pt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pd[idx.l3]) & ~PAGE_FLAGS_MASK);

      const uint64_t newValue = (physical_address & ~PAGE_FLAGS_MASK) | PAGE_PRESENT | flags
                                | static_cast<uint64_t>(flags2) << PAGE_FLAGS2_SHIFT;
      if (pt[idx.l4] != 0 && pt[idx.l4] != newValue) {
        framebuffer::defaultVirtualConsole.appendFormattedText(
          "attempted to overwrite page table entry for %p from %p to %p",
          virtual_address, pt[idx.l4] & ~PAGE_FLAGS_MASK & ~PAGE_FLAGS2_MASK, newValue & ~PAGE_FLAGS_MASK & ~PAGE_FLAGS2_MASK);
        const auto oldFlags = pt[idx.l4] & (PAGE_FLAGS_MASK | PAGE_FLAGS2_MASK) & ~(PAGE_ACCESSED | PAGE_DIRTY);
        const auto newFlags = newValue & (PAGE_FLAGS_MASK | PAGE_FLAGS2_MASK) & ~(PAGE_ACCESSED|PAGE_DIRTY);
        if (oldFlags != newFlags) {
          framebuffer::defaultVirtualConsole.appendFormattedText(" old flags %s", tableFlagsToString(
                                                                   oldFlags & PAGE_FLAGS_MASK,
                                                                   (oldFlags & PAGE_FLAGS2_MASK) >> PAGE_FLAGS2_SHIFT));
          framebuffer::defaultVirtualConsole.appendFormattedText(" new flags %s", tableFlagsToString(
                                                                   newFlags & PAGE_FLAGS_MASK,
                                                                   (newFlags & PAGE_FLAGS2_MASK) >> PAGE_FLAGS2_SHIFT));
        }
        framebuffer::defaultVirtualConsole.appendText("\n");
        kpanic("page table entry already exists");
      }
      pt[idx.l4] = newValue;

      physical_address += PAGE_SIZE;
      virtual_address += PAGE_SIZE;
    }
  }
}
