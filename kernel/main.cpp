#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limine.h>

#include <framebuffer/VirtualConsole.h>
#include <memory/MemMap.h>
#include <serial/Serial.h>
#include <smbios/smbios.h>

#include "utils/panic.h"

// Set the base revision to 3, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

namespace {
  __attribute__((used, section(".limine_requests"))) volatile LIMINE_BASE_REVISION(3);
}

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

// namespace {
//
// __attribute__((used, section(".limine_requests")))
// volatile limine_framebuffer_request framebuffer_request = {
//     .id = LIMINE_FRAMEBUFFER_REQUEST,
//     .revision = 0,
//     .response = nullptr
// };
//
// }

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .cpp file, as seen fit.

namespace {
  __attribute__((used, section(".limine_requests_start"))) volatile LIMINE_REQUESTS_START_MARKER;

  __attribute__((used, section(".limine_requests_end"))) volatile LIMINE_REQUESTS_END_MARKER;
} // namespace

// The following stubs are required by the Itanium C++ ABI (the one we use,
// regardless of the "Itanium" nomenclature).
// Like the memory functions above, these stubs can be moved to a different .cpp file,
// but should not be removed, unless you know what you are doing.
extern "C" {
int __cxa_atexit(void (*)(void *), void *, void *) { return 0; } // NOLINT(*-reserved-identifier)
void __cxa_pure_virtual() { halt(); } // NOLINT(*-reserved-identifier)
void *__dso_handle; // NOLINT(*-reserved-identifier)
}

// Extern declarations for global constructors array.
extern void (*__init_array[])(); // NOLINT(*-reserved-identifier)
extern void (*__init_array_end[])(); // NOLINT(*-reserved-identifier)

namespace {
  __attribute__((used, section(".limine_requests"))) volatile limine_smp_request smpMap = {
      .id = LIMINE_SMP_REQUEST, .revision = 0, .response = nullptr};
}

namespace {
  __attribute__((used, section(".limine_requests"))) volatile limine_dtb_request dtb = {
      .id = LIMINE_DTB_REQUEST, .revision = 0, .response = nullptr};
}

namespace {
  __attribute__((used, section(".limine_requests"))) volatile limine_efi_system_table_request efi_system_table = {
      .id = LIMINE_EFI_SYSTEM_TABLE_REQUEST, .revision = 0, .response = nullptr};
}

namespace {
  __attribute__((used, section(".limine_requests"))) volatile limine_rsdp_request rsdp = {
      .id = LIMINE_RSDP_REQUEST, .revision = 0, .response = nullptr};
}

struct XSDP_t {
  char Signature[8];
  uint8_t Checksum;
  char OEMID[6];
  uint8_t Revision;
  uint32_t RsdtAddress; // deprecated since version 2.0

  uint32_t Length;
  uint64_t XsdtAddress;
  uint8_t ExtendedChecksum;
  uint8_t reserved[3];
} __attribute__((packed));

namespace memory {
  extern volatile limine_hhdm_request hhdm_request;
}

// The following will be our kernel's entry point.
// If renaming kmain() to something else, make sure to change the
// linker script accordingly.
extern "C" void kmain() {
  // Ensure the bootloader actually understands our base revision (see spec).
  if (LIMINE_BASE_REVISION_SUPPORTED == false) {
    kpanic("limine base revision not supported");
  }

  // Call global constructors.
  for (std::size_t i = 0; &__init_array[i] != __init_array_end; i++) {
    __init_array[i]();
  }

  framebuffer::defaultVirtualConsole.init();
  memory::memMap.init();
  serial::defaultSerial.init(memory::hhdm_request.response->offset);

  if (dtb.response != nullptr) {
    kprintf("DTB at %p\n", dtb.response->dtb_ptr);
  } else {
    kprint("No DTB\n");
  }

  if (efi_system_table.response != nullptr) {
    kprintf("EFI system table at %p\n", efi_system_table.response->address);
  } else {
    kprint("No EFI system table\n");
  }

  if (rsdp.response != nullptr) {
    kprintf("RSDP at %p\n", rsdp.response->address);
  } else {
    kprint("No RSDP\n");
  }

  smbios::defaultSMBIOS.init(memory::hhdm_request.response->offset);
  kprint("start complete\n");
  halt();
}
