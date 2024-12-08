#include "paging.h"
#include <cstdio>
#include <cstring>
#include <memutil.h>
#include "framebuffer/VirtualConsole.h"
#include "limine.h"
#include "utils/bytes.h"
#include "utils/panic.h"

namespace memory {
  __attribute__((used, section(".limine_requests"))) volatile limine_paging_mode_request paging_request = {
      .id = LIMINE_PAGING_MODE_REQUEST,
      .revision = 0,
      .response = nullptr,
      .mode = LIMINE_PAGING_MODE_AARCH64_4LVL,
      .max_mode = LIMINE_PAGING_MODE_AARCH64_4LVL,
      .min_mode = LIMINE_PAGING_MODE_AARCH64_4LVL,
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

  void Paging::init(size_t count, limine_memmap_entry **mappings, uint64_t hhdmVirtualOffset,
                    uint64_t kernelVirtualOffset) {
    if (hhdmVirtualOffset % PAGE_SIZE != 0) {
      kpanic("virtual offset must be page aligned");
    }
    if (paging_request.response == nullptr) {
      kpanic("paging request not set");
    }
    if (paging_request.response->mode != LIMINE_PAGING_MODE_AARCH64_4LVL) {
      kpanic("paging mode not supported");
    }
    uint64_t ttbr0, ttbr1;
    asm volatile("mrs %0, ttbr0_el1; mrs %1, ttbr1_el1; mrs %2, tcr_el1" : "=r"(ttbr0), "=r"(ttbr1), "=r"(tcr_el1));

    Paging tmpPaging;
    tmpPaging.pageTableOffset = hhdmVirtualOffset;
    tmpPaging.tcr_el1 = tcr_el1;
    tmpPaging.higherHalfOffset = hhdmVirtualOffset;
    tmpPaging.root1 = reinterpret_cast<uint64_t *>(tmpPaging.adjustPageTablePhysicalToVirtual(ttbr0));
    tmpPaging.root2 = reinterpret_cast<uint64_t *>(tmpPaging.adjustPageTablePhysicalToVirtual(ttbr1));

    kprintf("current ttbr0: %p/%p ttbr1: %p/%p tcr_el1: %lx initial pool %p, hhdmVirtualOffset %p, kernelOffset %p\n",
            toPtr(ttbr0), toPtr(tmpPaging.root1), toPtr(ttbr1), toPtr(tmpPaging.root2), tcr_el1, toPtr(initialPool),
            toPtr(hhdmVirtualOffset), toPtr(kernelVirtualOffset));

    root1 = static_cast<uint64_t *>(GetPagePtr(1));
    root2 = static_cast<uint64_t *>(GetPagePtr(1));
    pageTableOffset = kernelVirtualOffset;
    memset(root1, 0, PAGE_SIZE);
    memset(root2, 0, PAGE_SIZE);

    // for (int i = 0; i < count; i++) {
    //   if (mappings[i]->type == LIMINE_MEMMAP_FRAMEBUFFER) {
    //     mapMemory(mappings[i]->base, framebuffer::defaultFramebuffer.getFramebufferVirtualAddress(),
    //               mappings[i]->length, PAGE_VALID);
    //     break;
    //   }
    // }

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
        return type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE || type == LIMINE_MEMMAP_KERNEL_AND_MODULES ||
               type == LIMINE_MEMMAP_FRAMEBUFFER;
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
            kprintf("block %p-%p/%p-%p %lu(%s) %s %lu\n", toPtr(page_table_range_data->virtualStart),
                    toPtr(page_table_range_data->virtualEnd), toPtr(page_table_range_data->physicalStart),
                    toPtr(page_table_range_data->physicalEnd), page_table_range_data->pageCount,
                    bytesToHumanReadable(buff, sizeof(buff), page_table_range_data->pageCount * PAGE_SIZE),
                    tableFlagsToString(page_table_range_data->flags), data->mappings[i]->type);
            data->paging->mapMemory(page_table_range_data->physicalStart, page_table_range_data->virtualStart,
                                    page_table_range_data->pageSize, page_table_range_data->pageCount,
                                    page_table_range_data->flags);
            mapped = true;
            break;
          }
        }
      }
      if (!mapped && page_table_range_data->pageCount > 2) {
        char buff[32];
        kprintf("not mapping %p-%p/%p-%p %lu(%s) %s\n", toPtr(page_table_range_data->virtualStart),
                toPtr(page_table_range_data->virtualEnd), toPtr(page_table_range_data->physicalStart),
                toPtr(page_table_range_data->physicalEnd), page_table_range_data->pageCount,
                bytesToHumanReadable(buff, sizeof(buff), page_table_range_data->pageCount * PAGE_SIZE),
                tableFlagsToString(page_table_range_data->flags));
      }
    };
    mapPhysicalToVirtual<callbackData> mapper = [](const uint64_t v, callbackData *d) {
      return reinterpret_cast<void *>(v + d->hhdmVirtualOffset);
    };
    tmpPaging.pageTableToRanges(mapper, callback, &data);
    kprintf("new paging table created at %p/%p using %d temporary tables\n", toPtr(root1), toPtr(root2),
            initialPoolUsedCount);
    asm volatile("msr ttbr0_el1, %0\n"
      "msr ttbr1_el1, %1\n"
      :
      : "r"(reinterpret_cast<uint64_t>(root1) - kernelVirtualOffset),
      "r"(reinterpret_cast<uint64_t>(root2) - kernelVirtualOffset)
      : "memory");
    invalidateCache();
    kprint("paging enabled\n");
  }

  void Paging::invalidateCache() {
    asm volatile("dsb ish\n" // Data Synchronization Barrier
      "isb\n" // Instruction Synchronization Barrier
      "tlbi vmalle1is\n" // Invalidate all TLB entries
      "dsb ish\n" // Data Synchronization Barrier
      "isb\n" // Instruction Synchronization Barrier
      ::
      : "memory");
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
                         const size_t num_pages, uint64_t flags) {
    kprintf("mapping %p-%p to %p-%p (%lu) %s", toPtr(virtual_address),
            toPtr(virtual_address + (num_pages * pageSize) - 1), toPtr(physical_address),
            toPtr(physical_address + (num_pages * pageSize) - 1), num_pages, tableFlagsToString(flags));
    if (physical_address % pageSize != 0) {
      kpanic("physical address must be page aligned");
    }
    if (virtual_address % pageSize != 0) {
      kpanicf("virtual address(%p) must be page aligned", toPtr(virtual_address));
    }

    for (size_t i = 0; i < num_pages; ++i) {
      const auto idx = virtualToPageIndexes(virtual_address);
      const auto root = idx.higherHalf ? root2 : root1;

      if (!(root[idx.l1] & PAGE_VALID)) {
        auto *new_pdpt = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pdpt, 0, PAGE_SIZE);
        root[idx.l1] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pdpt)) | PAGE_VALID | PAGE_TABLE;
      }
      const auto pdpt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(root[idx.l1] & ~PAGE_FLAGS_MASK));

      if (!(pdpt[idx.l2] & PAGE_VALID)) {
        auto *new_pd = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pd, 0, PAGE_SIZE);
        pdpt[idx.l2] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pd)) | PAGE_VALID | PAGE_TABLE;
      }
      const auto pd = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pdpt[idx.l2] & ~PAGE_FLAGS_MASK));
      if (pageSize == PAGE_SIZE * PAGE_ENTRIES) {
        kassertf(virtual_address % (PAGE_SIZE * PAGE_ENTRIES) == 0, "huge page must be page aligned: %p 0x%x",
                 toPtr(virtual_address), static_cast<unsigned>(virtual_address % pageSize));
        kassertf(physical_address % (PAGE_SIZE * PAGE_ENTRIES) == 0, "huge page must be page aligned: %p 0x%x",
                 toPtr(physical_address), static_cast<unsigned>(physical_address % pageSize));
        setPageTableEntry(pd, idx.l3, 3, virtual_address, physical_address, flags);
        virtual_address += pageSize;
        physical_address += pageSize;
        continue;
      }
      if (!(pd[idx.l3] & PAGE_VALID)) {
        auto *new_pt = static_cast<uint64_t *>(GetPagePtr(1));
        memset(new_pt, 0, PAGE_SIZE);
        pd[idx.l3] = adjustPageTableVirtualToPhysical(reinterpret_cast<uint64_t>(new_pt)) | PAGE_VALID | PAGE_TABLE;
      } else if ((pd[idx.l3] & PAGE_TABLE) == 0) {
        kpanicf("huge page already mapped at %p", toPtr(virtual_address));
      }
      const auto pt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pd[idx.l3] & ~PAGE_FLAGS_MASK));
      setPageTableEntry(pt, idx.l4, 4, virtual_address, physical_address, flags);
      physical_address += pageSize;
      virtual_address += pageSize;
    }
    invalidateCache();
    kprint(" done\n");
  }

  void Paging::unmapMemory(uint64_t virtual_address, size_t num_pages, size_t pageSize) {
    kprintf("unmapping %p-%p", toPtr(virtual_address), toPtr(virtual_address + (num_pages * pageSize) - 1));
    if (virtual_address % PAGE_SIZE != 0) {
      kprintf("\nvirtual address %p is not page aligned %lu\n", toPtr(virtual_address), virtual_address % PAGE_SIZE);
      kpanic("virtual address must be page aligned");
    }
    for (size_t i = 0; i < num_pages; ++i) {
      const auto idx = virtualToPageIndexes(virtual_address);
      const auto root = idx.higherHalf ? root2 : root1;

      if (!(root[idx.l1] & PAGE_VALID)) {
        virtual_address += pageSize;
        continue;
      }
      const auto pdpt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(root[idx.l1] & ~PAGE_FLAGS_MASK));

      if (!(pdpt[idx.l2] & PAGE_VALID)) {
        virtual_address += pageSize;
        continue;
      }
      const auto pd = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pdpt[idx.l2] & ~PAGE_FLAGS_MASK));
      if (pageSize == PAGE_SIZE * PAGE_ENTRIES) {
        kassertf(virtual_address % pageSize == 0, "huge page must be page aligned: %p 0x%x", toPtr(virtual_address),
                 static_cast<unsigned>(virtual_address % pageSize));
        kassertf((pd[idx.l3] & PAGE_TABLE) == 0, "huge page must not have a page table: %p", toPtr(virtual_address));
        clearPageTableEntry(pd, idx.l3);
        virtual_address += pageSize;
        continue;
      }
      if (!(pd[idx.l3] & PAGE_VALID)) {
        virtual_address += pageSize;
        continue;
      } else if ((pd[idx.l3] & PAGE_TABLE) == 0) {
        virtual_address += pageSize;
        continue;
      }
      const auto pt = reinterpret_cast<uint64_t *>(adjustPageTablePhysicalToVirtual(pd[idx.l3] & ~PAGE_FLAGS_MASK));
      clearPageTableEntry(pt, idx.l4);
      virtual_address += pageSize;
    }
    invalidateCache();
    kprint(" done\n");
  }

  uint64_t Paging::pageIndexesToVirtual(const uint64_t l[], const size_t count, const bool higherHalf) const {
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
    if (higherHalf) {
      address += higherHalfOffset;
    }
    return address;
  }

  Paging::virtualToPageIndexes_t Paging::virtualToPageIndexes(const uint64_t virtualAddress) const {
    return {virtualAddress >= higherHalfOffset, (virtualAddress >> 39) & 0x1FF, (virtualAddress >> 30) & 0x1FF,
            (virtualAddress >> 21) & 0x1FF, (virtualAddress >> 12) & 0x1FF};
  }

  char *Paging::tableFlagsToString(const uint64_t flags) {
    static char buf[32];
    size_t n = 0;
    if (flags & PAGE_VALID) {
      buf[n++] = 'V';
    }
    if (flags & PAGE_TABLE) {
      buf[n++] = 'T';
    }
    if (flags & ~(PAGE_VALID | PAGE_TABLE)) {
      n += ksnprintf(buf + n, sizeof(buf) - n, "%lx", flags & ~(PAGE_VALID | PAGE_TABLE));
    }
    buf[n] = 0;
    return buf;
  }

  void Paging::setPageTableEntry(uint64_t *table, const uint16_t index, const uint8_t level,
                                 const uint64_t virtualAddress,
                                 const uint64_t physicalAddress, const uint64_t flags) {
    const uint64_t newValue = (physicalAddress & PAGE_ADDR_MASK) | PAGE_VALID | flags | (level == 4 ? PAGE_TABLE : 0);
    if (table[index] != 0 && table[index] != newValue) {
      kprintf("attempted to overwrite page table entry for %p from %p to %p", toPtr(virtualAddress),
              toPtr(table[index] & ~PAGE_FLAGS_MASK), toPtr(newValue & ~PAGE_FLAGS_MASK));
      const auto oldFlags = table[index] & PAGE_FLAGS_MASK & ~(PAGE_ACCESSED | PAGE_DIRTY);
      const auto newFlags = newValue & PAGE_FLAGS_MASK & ~(PAGE_ACCESSED | PAGE_DIRTY);
      if (oldFlags != newFlags) {
        kprintf(" old flags %s", tableFlagsToString(oldFlags & PAGE_FLAGS_MASK));
        kprintf(" new flags %s", tableFlagsToString(newFlags & PAGE_FLAGS_MASK));
      }
      kprint("\n");
      kpanic("page table entry already exists");
    }
    table[index] = newValue;
  }

  void Paging::clearPageTableEntry(uint64_t *table, const uint16_t index) { table[index] = 0; }
} // namespace memory
