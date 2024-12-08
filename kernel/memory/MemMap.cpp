#include "MemMap.h"
#include <cstdio>
#include <framebuffer/VirtualConsole.h>
#include <limine.h>
#include <memutil.h>
#include "memory/paging.h"
#include "utils/bytes.h"

namespace memory {
  __attribute__((used, section(".limine_requests"))) volatile limine_memmap_request memMapRequest = {
      .id = LIMINE_MEMMAP_REQUEST, .revision = 0, .response = nullptr};
  __attribute__((used, section(".limine_requests"))) volatile limine_kernel_address_request kernel_address = {
      .id = LIMINE_KERNEL_ADDRESS_REQUEST, .revision = 0, .response = nullptr};
  __attribute__((used, section(".limine_requests"))) volatile limine_hhdm_request hhdm_request = {
      .id = LIMINE_HHDM_REQUEST, .revision = 0, .response = nullptr};

  MemMap memMap;

  const char *memTypeName[] = {"usable",
                               "reserved",
                               "ACPI reclaimable",
                               "ACPI NVS",
                               "bad memory",
                               "bootloader reclaimable",
                               "kernel and modules",
                               "framebuffer"};

  const char *getMemMapTypeDescription(uint64_t typeCode) {
    if (typeCode < sizeof(memTypeName) / sizeof(memTypeName[0])) {
      return memTypeName[typeCode];
    }
    return "unknown";
  }

  void MemMap::init() {
    uint64_t perTypeMemory[LIMINE_MEMMAP_FRAMEBUFFER + 1] = {};
    kprintf("%lu mem map entries\n", memMapRequest.response->entry_count);
    kprintf("kernel at %p/%p stack at %p hhdm offset %p\n",
            reinterpret_cast<void *>(kernel_address.response->physical_base),
            reinterpret_cast<void *>(kernel_address.response->virtual_base), reinterpret_cast<void *>(perTypeMemory),
            reinterpret_cast<void *>(hhdm_request.response->offset));
    kprintf("limine response pointer is %p/%p\n", reinterpret_cast<void *>(kernel_address.response),
            subFromPointer<void>(kernel_address.response, hhdm_request.response->offset));

    for (int i = 0; i < memMapRequest.response->entry_count; i++) {
      const auto e = memMapRequest.response->entries[i];
      if (e->type <= LIMINE_MEMMAP_FRAMEBUFFER) {
        perTypeMemory[e->type] += e->length;
      }
      if (e->type != LIMINE_MEMMAP_RESERVED) {
        char buf[256];
        bytesToHumanReadable(buf, sizeof(buf), e->length);
        kprintf("Entry %d: %p - %p %s %s\n", i, reinterpret_cast<void *>(e->base),
                reinterpret_cast<void *>(e->base + e->length), buf, getMemMapTypeDescription(e->type));
      }
    }

    for (int i = 0; i < sizeof(perTypeMemory) / sizeof(perTypeMemory[0]); i++) {
      if (perTypeMemory[i] > 0) {
        char buf[256] = {};
        kprintf("%s: %s\n", getMemMapTypeDescription(i), bytesToHumanReadable(buf, sizeof(buf), perTypeMemory[i]));
      }
    }
    paging.init(memMapRequest.response->entry_count, memMapRequest.response->entries, hhdm_request.response->offset,
                kernel_address.response->virtual_base - kernel_address.response->physical_base);
  }
} // namespace memory
