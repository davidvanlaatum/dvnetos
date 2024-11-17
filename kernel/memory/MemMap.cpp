#include <limine.h>
#include <cstdio>
#include <framebuffer/VirtualConsole.h>
#include "MemMap.h"
#include "memory/paging.h"
#include "utils/bytes.h"
#include "utils/panic.h"

namespace memory {
    __attribute__((used, section(".limine_requests")))
    volatile limine_memmap_request memMapRequest = {
        .id = LIMINE_MEMMAP_REQUEST,
        .revision = 0,
        .response = nullptr
    };
    __attribute__((used, section(".limine_requests")))
    volatile limine_kernel_address_request kernel_address = {
        .id = LIMINE_KERNEL_ADDRESS_REQUEST,
        .revision = 0,
        .response = nullptr
    };
    __attribute__((used, section(".limine_requests")))
    volatile limine_hhdm_request hhdm_request = {
        .id = LIMINE_HHDM_REQUEST,
        .revision = 0,
        .response = nullptr
    };

    MemMap memMap;

    const char *memTypeName[] = {
        "usable",
        "reserved",
        "ACPI reclaimable",
        "ACPI NVS",
        "bad memory",
        "bootloader reclaimable",
        "kernel and modules",
        "framebuffer"
    };

    const char *getMemMapTypeDescription(uint64_t typeCode) {
        if (typeCode < sizeof(memTypeName) / sizeof(memTypeName[0])) {
            return memTypeName[typeCode];
        }
        return "unknown";
    }

    void MemMap::init() {
        char buf[256] = {};
        ksnprintf(buf, sizeof(buf), "%d mem map entries\n", memMapRequest.response->entry_count);
        framebuffer::defaultVirtualConsole.appendText(buf);

        ksnprintf(buf, sizeof(buf), "kernel at %p/%p stack at %p hhdm offset %p\n",
                  kernel_address.response->physical_base,
                  kernel_address.response->virtual_base, buf, hhdm_request.response->offset);
        framebuffer::defaultVirtualConsole.appendText(buf);

        uint64_t perTypeMemory[LIMINE_MEMMAP_FRAMEBUFFER + 1] = {};
        // InitialMapping mappings[4];
        // int freeMemIndex = -1;
        // int kernelMemIndex = -1;
        // int framebufferMemIndex = -1;
        // int stackMemIndex = -1;
        // int responsesIndex = -1;
        // const uint64_t stackPhysicalAddress = reinterpret_cast<uint64_t>(buf) - hhdm_request.response->offset;

        framebuffer::defaultVirtualConsole.appendFormattedText("limine response pointer is %p/%p\n",
                                                               kernel_address.response,
                                                               reinterpret_cast<uint64_t>(kernel_address.response) -
                                                               hhdm_request.response->offset);

        for (int i = 0; i < memMapRequest.response->entry_count; i++) {
            const auto e = memMapRequest.response->entries[i];
            if (e->type <= LIMINE_MEMMAP_FRAMEBUFFER) {
                perTypeMemory[e->type] += e->length;
            }
            if (e->type != LIMINE_MEMMAP_RESERVED) {
                char buf2[256];
                ksnprintf(buf, sizeof(buf), "Entry %d: %p - %p %s %s\n", i, e->base, e->base + e->length,
                          bytesToHumanReadable(buf2, sizeof(buf2), e->length), getMemMapTypeDescription(e->type));
                framebuffer::defaultVirtualConsole.appendText(buf);
            }

            // if (e->type == LIMINE_MEMMAP_USABLE && freeMemIndex == -1 && e->length >= PAGE_SIZE * 64 && e->base > 0) {
            //     freeMemIndex = i;
            // } else if (e->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            //     framebufferMemIndex = i;
            // } else if (e->type == LIMINE_MEMMAP_KERNEL_AND_MODULES) {
            //     kernelMemIndex = i;
            // } else if (e->base <= stackPhysicalAddress && e->base + e->length >= stackPhysicalAddress) {
            //     ksnprintf(buf, sizeof(buf), "Entry %d is the stack\n", i);
            //     framebuffer::defaultVirtualConsole.appendText(buf);
            //     stackMemIndex = i;
            // }
            // if (e->base <= reinterpret_cast<uint64_t>(kernel_address.response) - hhdm_request.response->offset &&
            //            e->base + e->length >=
            //            reinterpret_cast<uint64_t>(kernel_address.response) - hhdm_request.response->offset) {
            //     framebuffer::defaultVirtualConsole.appendFormattedText("Entry %d is the limine responses\n", i);
            // }
        }

        for (int i = 0; i < sizeof(perTypeMemory) / sizeof(perTypeMemory[0]); i++) {
            if (perTypeMemory[i] > 0) {
                char buf2[256] = {};
                ksnprintf(buf, sizeof(buf), "%s: %s\n", getMemMapTypeDescription(i),
                          bytesToHumanReadable(buf2, sizeof(buf2), perTypeMemory[i]));
                framebuffer::defaultVirtualConsole.appendText(buf);
            }
        }

        // if (kernelMemIndex == -1) {
        //     kpanic("No kernel memory found");
        // }
        // mappings[0] = {
        //     .physicalAddress = memMapRequest.response->entries[kernelMemIndex]->base,
        //     .virtualAddress = kernel_address.response->virtual_base,
        //     .size = memMapRequest.response->entries[kernelMemIndex]->length
        // };
        // if (stackMemIndex == -1) {
        //     kpanic("No stack memory found");
        // }
        // mappings[1] = {
        //     .physicalAddress = memMapRequest.response->entries[stackMemIndex]->base,
        //     .virtualAddress = memMapRequest.response->entries[stackMemIndex]->base + hhdm_request.response->offset,
        //     .size = memMapRequest.response->entries[stackMemIndex]->length
        // };
        // if (framebufferMemIndex == -1) {
        //     kpanic("No framebuffer memory found");
        // }
        // mappings[2] = {
        //     .physicalAddress = memMapRequest.response->entries[framebufferMemIndex]->base,
        //     .virtualAddress = framebuffer::defaultFramebuffer.getFramebufferVirtualAddress(),
        //     .size = memMapRequest.response->entries[framebufferMemIndex]->length
        // };
        // if (freeMemIndex == -1) {
        //     kpanic("No free memory found");
        // }
        // mappings[3] = {
        //     .physicalAddress = memMapRequest.response->entries[freeMemIndex]->base,
        //     .virtualAddress = 0,
        //     .size = PAGE_SIZE * 64
        // };
        // for (int i = 0; i < sizeof(mappings) / sizeof(mappings[0]) - 1; i++) {
        //     if (mappings[3].virtualAddress < mappings[i].virtualAddress + mappings[i].size) {
        //         mappings[3].virtualAddress = mappings[i].virtualAddress + mappings[i].size;
        //     }
        // }
        paging.init(memMapRequest.response->entry_count, memMapRequest.response->entries, hhdm_request.response->offset,
                    kernel_address.response->virtual_base - kernel_address.response->physical_base);
    }
}
