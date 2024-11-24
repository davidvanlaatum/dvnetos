#include <cstring>
#include "limine.h"
#include "framebuffer/VirtualConsole.h"
#include "utils/bytes.h"
#include "utils/panic.h"
#include "paging.h"

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

  Paging paging;

  constexpr size_t INITIAL_POOL_SIZE = 128;
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

  char *Paging::tableFlagsToString(const uint64_t flags) {
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
    if (flags & PAGE_GLOBAL) {
      buf[n++] = 'G';
    }
    if (flags & PAGE_NX) {
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

    struct callbackData {
      size_t mapCount;
      limine_memmap_entry **mappings;
      Paging *paging;
      uint64_t hhdmVirtualOffset;
    };
    callbackData data = {count, mappings, this, hhdmVirtualOffset};
    pageTableToRangesCallback<callbackData> callback = [](PageTableRangeData *page_table_range_data,
                                                          callbackData *data) {
      static auto isTypeToMap = [](const uint64_t type) {
        return type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE
               || type == LIMINE_MEMMAP_KERNEL_AND_MODULES
               || type == LIMINE_MEMMAP_FRAMEBUFFER;
      };
      static auto rangesOverlap = [](PageTableRangeData *page_table_range_data, limine_memmap_entry *entry) {
        return (page_table_range_data->physicalStart <= entry->base + entry->length &&
                page_table_range_data->physicalEnd >= entry->base) ||
               (page_table_range_data->physicalStart <= entry->base &&
                page_table_range_data->physicalEnd >= entry->base) ||
               (page_table_range_data->physicalStart <= entry->base + entry->length &&
                page_table_range_data->physicalEnd >= entry->base + entry->length);
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
              tableFlagsToString(page_table_range_data->flags),
              data->mappings[i]->type);
            data->paging->mapMemory(page_table_range_data->physicalStart,
                                    page_table_range_data->virtualStart,
                                    page_table_range_data->pageSize,
                                    page_table_range_data->pageCount,
                                    page_table_range_data->flags);
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
          tableFlagsToString(page_table_range_data->flags));
      }
    };
    mapPhysicalToVirtual<callbackData> mapper = [](const uint64_t v, callbackData *d) {
      return reinterpret_cast<void *>(v + d->hhdmVirtualOffset);
    };
    pageTableToRanges(reinterpret_cast<uint64_t *>(cr3 + hhdmVirtualOffset), mapper, callback, &data);

    uint64_t rootPhysicalAddress = reinterpret_cast<uint64_t>(root) - kernelVirtualOffset;
    framebuffer::defaultVirtualConsole.appendFormattedText(
      "new paging table created at %p/%p using %d temporary tables\n", root,
      rootPhysicalAddress, initialPoolUsedCount);
    asm volatile("mov %0, %%cr3" : : "r"(rootPhysicalAddress) : "memory");
    framebuffer::defaultVirtualConsole.appendText("paging enabled\n");
  }

  void Paging::mapPartial(const uint64_t physical_address, const uint64_t virtual_address, const size_t size,
                          const uint64_t flags) {
    const auto new_physical_address = makePageAligned(physical_address);
    const auto new_virtual_address = makePageAligned(virtual_address);
    auto new_size = size + PAGE_SIZE - (size % PAGE_SIZE);
    if (physical_address + size > new_physical_address + new_size) {
      new_size += PAGE_SIZE;
    }
    mapMemory(new_physical_address, new_virtual_address, PAGE_SIZE, new_size / PAGE_SIZE, flags);
  }

  void Paging::mapMemory(uint64_t physical_address, uint64_t virtual_address, const size_t pageSize,
                         const size_t num_pages,
                         const uint64_t flags) {
    framebuffer::defaultVirtualConsole.appendFormattedText(
      "mapping %p-%p to %p-%p (%d) %s\n", virtual_address, virtual_address + (num_pages * PAGE_SIZE) - 1,
      physical_address, physical_address + (num_pages * PAGE_SIZE) - 1, num_pages, tableFlagsToString(flags));
    if (physical_address % PAGE_SIZE != 0) {
      kpanic("physical address must be page aligned");
    }
    if (virtual_address % PAGE_SIZE != 0) {
      framebuffer::defaultVirtualConsole.appendFormattedText("virtual address %p is not page aligned %d\n",
                                                             virtual_address, virtual_address % PAGE_SIZE);
      kpanic("virtual address must be page aligned");
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

      if (pageSize == PAGE_SIZE * PAGE_ENTRIES) {
        kassertf(virtual_address % (PAGE_SIZE * PAGE_ENTRIES) == 0, "huge page must be page aligned: %p 0x%x",
                 virtual_address, virtual_address % (PAGE_SIZE * PAGE_ENTRIES));
        // kassertf(physical_address % (PAGE_SIZE * PAGE_ENTRIES) == 0, "huge page must be page aligned: %p 0x%x",
        //         physical_address, physical_address % (PAGE_SIZE * PAGE_ENTRIES));
        framebuffer::defaultVirtualConsole.appendFormattedText(
          "mapping huge page at %p-%p/%p-%p\n", virtual_address, virtual_address + (PAGE_SIZE * PAGE_ENTRIES) - 1,
          physical_address, physical_address + (PAGE_SIZE * PAGE_ENTRIES) - 1);
        setPageTableEntry(pd, idx.l3, virtual_address, physical_address, flags | PAGE_SIZE_FLAG);
        virtual_address += PAGE_SIZE * PAGE_ENTRIES;
        physical_address += PAGE_SIZE * PAGE_ENTRIES;
        continue;
      }
      if (!(pd[idx.l3] & PAGE_PRESENT)) {
        auto *new_pt = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pt, 0, PAGE_SIZE);
        pd[idx.l3] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pt)) | PAGE_PRESENT | PAGE_WRITE |
                     PAGE_USER;
      } else if (pd[idx.l3] & PAGE_SIZE_FLAG) {
        continue;
      }
      const auto pt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pd[idx.l3] & ~PAGE_FLAGS_MASK));
      setPageTableEntry(pt, idx.l4, virtual_address, physical_address, flags);
      physical_address += PAGE_SIZE;
      virtual_address += PAGE_SIZE;
    }
  }

  void Paging::setPageTableEntry(uint64_t *table, const uint16_t index, const uint64_t virtualAddress,
                                 const uint64_t physicalAddress, const uint64_t flags) {
    const uint64_t newValue = (physicalAddress & PAGE_ADDR_MASK) | PAGE_PRESENT | flags;
    if (table[index] != 0 && table[index] != newValue) {
      framebuffer::defaultVirtualConsole.appendFormattedText(
        "attempted to overwrite page table entry for %p from %p to %p",
        virtualAddress, table[index] & ~PAGE_FLAGS_MASK,
        newValue & ~PAGE_FLAGS_MASK);
      const auto oldFlags = table[index] & PAGE_FLAGS_MASK & ~(PAGE_ACCESSED | PAGE_DIRTY);
      const auto newFlags = newValue & PAGE_FLAGS_MASK & ~(PAGE_ACCESSED | PAGE_DIRTY);
      if (oldFlags != newFlags) {
        framebuffer::defaultVirtualConsole.appendFormattedText(" old flags %s", tableFlagsToString(
                                                                 oldFlags & PAGE_FLAGS_MASK));
        framebuffer::defaultVirtualConsole.appendFormattedText(" new flags %s", tableFlagsToString(
                                                                 newFlags & PAGE_FLAGS_MASK));
      }
      framebuffer::defaultVirtualConsole.appendText("\n");
      kpanic("page table entry already exists");
    }
    table[index] = newValue;
  }
}
