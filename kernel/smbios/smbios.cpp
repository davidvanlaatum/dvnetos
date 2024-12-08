#include "smbios.h"
#include <cstdio>
#include <framebuffer/VirtualConsole.h>
#include <limine.h>

#include "cstring"
#include "memory/paging.h"
#include "memutil.h"
#include "smbios_tostring.h"
#include "utils/bytes.h"
#include "utils/panic.h"

namespace {
  __attribute__((used, section(".limine_requests"))) volatile limine_smbios_request smbios_request = {
      .id = LIMINE_SMBIOS_REQUEST, .revision = 3, .response = nullptr};
}

using memory::toPtr;

namespace smbios {
  SMBIOS defaultSMBIOS;

  size_t smbiosStructLen(const TableHeader *hd) {
    size_t i;
    const char *strtab = reinterpret_cast<const char *>(hd) + hd->length;
    // Scan until we find a double zero byte
    for (i = 1; strtab[i - 1] != '\0' || strtab[i] != '\0'; i++) {
    }
    return hd->length + i + 1;
  }

  void SMBIOS::init(uint64_t hhdmOffset) {
    if (smbios_request.response != nullptr) {
      if (smbios_request.response->entry_64 != nullptr) {
        const auto *entry =
            reinterpret_cast<Entry64 *>(reinterpret_cast<uint64_t>(smbios_request.response->entry_64) + hhdmOffset);
        memory::paging.mapPartial(reinterpret_cast<uint64_t>(smbios_request.response->entry_64),
                                  reinterpret_cast<uint64_t>(entry), sizeof(Entry64), 0x700);
        kprintf("SMBIOS 64-bit entry: %d.%d table at %p length %d\n", static_cast<int>(entry->major_version),
                static_cast<int>(entry->minor_version), toPtr(entry->table_address), entry->table_length);
        const auto *table = reinterpret_cast<TableHeader *>(static_cast<uint64_t>(entry->table_address) + hhdmOffset);
        memory::paging.mapPartial(entry->table_address, reinterpret_cast<uint64_t>(table), entry->table_length, 0x700);
        dumpTable(table, entry->table_length);
      } else if (smbios_request.response->entry_32 != nullptr) {
        const auto *entry =
            reinterpret_cast<Entry32 *>(reinterpret_cast<uint64_t>(smbios_request.response->entry_32) + hhdmOffset);
        memory::paging.mapPartial(reinterpret_cast<uint64_t>(smbios_request.response->entry_32),
                                  reinterpret_cast<uint64_t>(entry), sizeof(Entry32), 0);
        kprintf("SMBIOS 32-bit entry: %d.%d table at %p length %d\n", static_cast<int>(entry->major_version),
                static_cast<int>(entry->minor_version), toPtr(entry->table_address), entry->table_length);
        const auto *table = reinterpret_cast<TableHeader *>(static_cast<uint64_t>(entry->table_address) + hhdmOffset);
        memory::paging.mapPartial(entry->table_address, reinterpret_cast<uint64_t>(table), entry->table_length, 0);
        dumpTable(table, entry->table_length);
      } else {
        kprint("No SMBIOS address\n");
      }
    } else {
      kprint("No SMBIOS\n");
    }
    kprint("SMBIOS end\n");
  }


  void SMBIOS::dumpTable(const TableHeader *table, const uint16_t length) {
    for (auto ptr = table; reinterpret_cast<uint64_t>(ptr) < reinterpret_cast<uint64_t>(table) + length;
         ptr = reinterpret_cast<TableHeader *>(reinterpret_cast<uint64_t>(ptr) + smbiosStructLen(ptr))) {
      char buf[512];
      switch (ptr->type)
      case TableType::FirmwareInfo: {
        kprintf("%s\n", reinterpret_cast<const FirmwareInfo *>(ptr)->toString(buf, sizeof(buf)));
        break;
        case TableType::SystemInfo:
          kprintf("%s\n", reinterpret_cast<const SystemInfo *>(ptr)->toString(buf, sizeof(buf)));
          break;
        case TableType::ChassisInfo:
          kprintf("%s\n", reinterpret_cast<const ChassisInfo *>(ptr)->toString(buf, sizeof(buf)));
          break;
        case TableType::ProcessorInfo:
          kprintf("%s\n", reinterpret_cast<const ProcessorInfo *>(ptr)->toString(buf, sizeof(buf)));
          break;
        case TableType::PhysicalMemoryArray:
          kprintf("%s\n", reinterpret_cast<const PhysicalMemoryArray *>(ptr)->toString(buf, sizeof(buf)));
          break;
        case TableType::MemoryDevice:
          kprintf("%s\n", reinterpret_cast<const MemoryDevice *>(ptr)->toString(buf, sizeof(buf)));
          break;
        case TableType::EndOfTable:
          kprint("End of table\n");
          return;
        default:
          kprintf("Unhandled type %s length %d\n", toString(ptr->type), ptr->length);
      }
    }
  }

  const char *getString(const TableHeader *entry, const uint8_t index) {
    if (index == 0) {
      return nullptr;
    }
    auto *strtab = reinterpret_cast<const char *>(entry) + entry->length;
    for (uint8_t i = 1; i < index; i++) {
      while (*strtab != '\0') {
        strtab++;
      }
      strtab++;
    }
    return strtab;
  }

#define offsetof(member) (reinterpret_cast<uint64_t>(&this->member) - reinterpret_cast<uint64_t>(this))

  const char *FirmwareInfo::toString(char *buf, size_t size) const {
    static char buf2[32];
    auto n =
        ksnprintf(buf, size, "Firmware vendor: %s version: %s release: %s rom size: %s", getString(this, this->vendor),
                  getString(this, this->version), getString(this, this->releaseDate),
                  bytesToHumanReadable(buf2, sizeof(buf2), this->romSize * 64));
    for (uint64_t i = 0; i < 64; i++) {
      if (this->characteristics & (1UL << i)) {
        n += ksnprintf(buf + n, size - n, " %s", ::smbios::toString(static_cast<FirmwareCharacteristics>(1UL << i)));
      }
    }
    if (offsetof(systemBiosMinorRelease) <= this->length) {
      n += ksnprintf(buf + n, size - n, " system bios: %d", this->systemBiosMajorRelease);
    }
    if (offsetof(ecFirmwareMinorRelease) <= this->length) {
      n += ksnprintf(buf + n, size - n, ".%d", this->systemBiosMinorRelease);
    }
    if (offsetof(ecFirmwareMinorRelease) <= this->length) {
      n += ksnprintf(buf + n, size - n, " firmware: %d", this->ecFirmwareMajorRelease);
    }
    if (offsetof(extendedFirmwareRomSize) <= this->length) {
      n += ksnprintf(buf + n, size - n, ".%d", this->ecFirmwareMinorRelease);
    }
    if (this->length == sizeof(FirmwareInfo)) {
      uint16_t units = (0x3 << 14) & this->extendedFirmwareRomSize;
      uint64_t bytes = this->extendedFirmwareRomSize & ~0x3;
      switch (units >> 14) {
        case 0:
          bytes *= 1024 * 1024;
          break;
        case 1:
          bytes *= 1024 * 1024 * 1024;
          break;
        default:
          kpanic("unknown units");
      }
      ksnprintf(buf + n, size - n, " extended Rom: %s", bytesToHumanReadable(buf2, sizeof(buf2), bytes));
    }
    return buf;
  }

  const char *SystemInfo::toString(char *buf, size_t size) const {
    ksnprintf(buf, size, "System vendor: %s product: %s version: %s serial: %s sku: %s family: %s",
              getString(this, this->vendor), getString(this, this->product), getString(this, this->version),
              getString(this, this->serial), getString(this, this->sku), getString(this, this->family));
    return buf;
  }

  const char *ChassisInfo::toString(char *buf, size_t size) const {
    auto n = ksnprintf(buf, size, "Chassis manufacturer: %s version: %s serial: %s asset: %s",
                       getString(this, this->manufacturer), getString(this, this->version),
                       getString(this, this->serial), getString(this, this->assetTag));
    if (this->length >= offsetof(powerSupplyState)) {
      n += ksnprintf(buf + n, size - n, " bootup State: %s", ::smbios::toString(this->bootUpState));
    }
    if (this->length >= offsetof(thermalState)) {
      n += ksnprintf(buf + n, size - n, " power Supply State: %s", ::smbios::toString(this->powerSupplyState));
    }
    if (this->length >= offsetof(securityStatus)) {
      n += ksnprintf(buf + n, size - n, " thermalState: %s", ::smbios::toString(this->thermalState));
    }
    if (this->length >= offsetof(oemDefined)) {
      n += ksnprintf(buf + n, size - n, " securityStatus: %s", ::smbios::toString(this->securityStatus));
    }
    if (this->length >= offsetof(height)) {
      n += ksnprintf(buf + n, size - n, " oemDefined: %d", this->oemDefined);
    }
    if (this->length >= offsetof(numberOfPowerCords)) {
      n += ksnprintf(buf + n, size - n, " height: %d", this->height);
    }
    if (this->length >= offsetof(containedElementCount)) {
      n += ksnprintf(buf + n, size - n, " powerCords: %d", this->numberOfPowerCords);
    }
    if (this->length >= offsetof(containedElementRecordLength)) {
      ksnprintf(buf + n, size - n, " containedElements: %d", this->containedElementCount);
    }
    return buf;
  }

  const char *ProcessorInfo::toString(char *buf, size_t size) const {
    auto n = ksnprintf(buf, size, "%s", this->getDesignation());
    n += ksnprintf(buf + n, size - n, " type: %s", ::smbios::toString(this->getType()));
    n += ksnprintf(buf + n, size - n, " family: %s", ::smbios::toString(this->getFamily()));
    if (const auto manufacturer = this->getManufacturer()) {
      n += ksnprintf(buf + n, size - n, " manufacturer: %s", manufacturer);
    }
    n += ksnprintf(buf + n, size - n, " id: %lu", this->getId());
    if (const auto version = this->getVersion()) {
      n += ksnprintf(buf + n, size - n, " version: %s", version);
    }
    if (const auto clock = this->getExternalClock()) {
      n += ksnprintf(buf + n, size - n, " externalClock: %dMHz", clock);
    }
    if (const auto clock = this->getMaxSpeed()) {
      n += ksnprintf(buf + n, size - n, " maxSpeed: %dMHz", clock);
    }
    if (const auto clock = this->getCurrentSpeed()) {
      n += ksnprintf(buf + n, size - n, " currentSpeed: %dMHz", clock);
    }
    n += ksnprintf(buf + n, size - n, " status: %s", this->isPopulated() ? "populated" : "unpopulated");
    n += ksnprintf(buf + n, size - n, " status: %s", this->getStatusString());
    n += ksnprintf(buf + n, size - n, " socket: %s", this->getSocket());
    if (const auto serial = this->getSerialNumber()) {
      n += ksnprintf(buf + n, size - n, " serial: %s", serial);
    }
    if (const auto asset = this->getAssetTag()) {
      n += ksnprintf(buf + n, size - n, " asset: %s", asset);
    }
    if (const auto part = this->getPartNumber()) {
      n += ksnprintf(buf + n, size - n, " part: %s", part);
    }
    if (const auto cores = this->getCores()) {
      n += ksnprintf(buf + n, size - n, " cores: %d", cores);
    }
    if (const auto cores = this->getCoresEnabled()) {
      n += ksnprintf(buf + n, size - n, " enabled: %d", cores);
    }
    if (const auto count = this->getThreads()) {
      n += ksnprintf(buf + n, size - n, " threads: %d", count);
    }
    return buf;
  }

  const char *ProcessorInfo::getDesignation() const { return getString(this, this->socketDesignation); }

  ProcessorType ProcessorInfo::getType() const { return this->processorType; }

  ProcessorFamily ProcessorInfo::getFamily() const { return this->processorFamily; }

  const char *ProcessorInfo::getManufacturer() const { return getString(this, this->processorManufacturer); }

  uint64_t ProcessorInfo::getId() const { return this->processorId; }

  const char *ProcessorInfo::getVersion() const { return getString(this, this->processorVersion); }

  uint16_t ProcessorInfo::getExternalClock() const { return this->externalClock; }

  uint16_t ProcessorInfo::getMaxSpeed() const { return this->maxSpeed; }

  uint16_t ProcessorInfo::getCurrentSpeed() const { return this->currentSpeed; }

  bool ProcessorInfo::isPopulated() const { return this->status & 1 << 6; }

  ProcessorStatus ProcessorInfo::getStatus() const { return static_cast<ProcessorStatus>(this->status & 0x7); }

  const char *ProcessorInfo::getStatusString() const {
    static const char *cpuStatusList[] = {
        "Unknown", // 0
        "CPU Enabled", // 1
        "CPU Disabled by User", // 2
        "CPU Disabled By BIOS (POST Error)", // 3
        "CPU Is Idle", // 4
        nullptr,
        nullptr,
        "Other", // 7
    };
    if (const auto status = static_cast<size_t>(this->getStatus());
        status < sizeof(cpuStatusList) / sizeof(cpuStatusList[0])) {
      return cpuStatusList[status];
    }
    return ::smbios::toString(this->getStatus());
  }

  const char *ProcessorInfo::getSerialNumber() const {
    if (offsetof(assetTag) <= this->length) {
      return getString(this, this->serialNumber);
    }
    return nullptr;
  }

  const char *ProcessorInfo::getAssetTag() const {
    if (offsetof(partNumber) <= this->length) {
      return getString(this, this->assetTag);
    }
    return nullptr;
  }

  const char *ProcessorInfo::getPartNumber() const {
    if (offsetof(coreCount) <= this->length) {
      return getString(this, this->partNumber);
    }
    return nullptr;
  }

  uint8_t ProcessorInfo::getCores() const {
    if (offsetof(coreEnabled) <= this->length) {
      return this->coreCount;
    }
    return 0;
  }

  uint8_t ProcessorInfo::getCoresEnabled() const {
    if (offsetof(threadCount) <= this->length) {
      return this->coreEnabled;
    }
    return 0;
  }

  uint8_t ProcessorInfo::getThreads() const {
    if (offsetof(processorCharacteristics) <= this->length) {
      return this->threadCount;
    }
    return 0;
  }

  const char *ProcessorInfo::getSocket() const {
    static const char *processorSocketsList[] = {
        nullptr,
        "Other", // 1
        "Unknown", // 2
        "Daughter Board", // 3
        "ZIF Socket", // 4
        "Replacement/Piggy Back", // 5
        "None", // 6
        "LIF Socket", // 7
        "Slot 1", // 8
        "Slot 2", // 9
        "370 Pin Socket", // 10
        "Slot A", // 11
        "Slot M", // 12
        "Socket 423", // 13
        "Socket A (Socket 462)", // 14
        "Socket 478", // 15
        "Socket 754", // 16
        "Socket 940", // 17
        "Socket 939", // 18
        "Socket mPGA604", // 19
        "Socket LGA771", // 20
        "Socket LGA775", // 21
        "Socket S1", // 22
        "Socket AM2", // 23
        "Socket F (1207)", // 24
        "Socket LGA1366", // 25
        "Socket G34", // 26
        "Socket AM3", // 27
        "Socket C32", // 28
        "Socket LGA1156", // 29
        "Socket LGA1567", // 30
        "Socket PGA988A", // 31
        "Socket BGA1288", // 32
        "rPGA988B", // 33
        "BGA1023", // 34
        "BGA1224", // 35
        "LGA1155", // 36
        "LGA1356", // 37
        "LGA2011", // 38
        "Socket FS1", // 39
        "Socket FS2", // 40
        "Socket FM1", // 41
        "Socket FM2", // 42
        "Socket LGA2011-3", // 43
        "Socket LGA1356-3", // 44
        "Socket LGA1150", // 45
        "Socket BGA1168", // 46
        "Socket BGA1234", // 47
        "Socket BGA1364", // 48
        "Socket AM4", // 49
        "Socket LGA1151", // 50
        "Socket BGA1356", // 51
        "Socket BGA1440", // 52
        "Socket BGA1515", // 53
        "Socket LGA3647-1", // 54
        "Socket SP3", // 55
        "Socket SP3r2", // 56
        "Socket LGA2066", // 57
        "Socket BGA1392", // 58
        "Socket BGA1510", // 59
        "Socket BGA1528", // 60
        "Socket LGA4189", // 61
        "Socket LGA1200", // 62
        "Socket LGA4677", // 63
        "Socket LGA1700", // 64
        "Socket BGA1744", // 65
        "Socket BGA1781", // 66
        "Socket BGA1211", // 67
        "Socket BGA2422", // 68
        "Socket LGA1211", // 69
        "Socket LGA2422", // 70
        "Socket LGA5773", // 71
        "Socket BGA5773", // 72
        "Socket AM5", // 73
        "Socket SP5", // 74
        "Socket SP6", // 75
        "Socket BGA883", // 76
        "Socket BGA1190", // 77
        "Socket LGA4129", // 78
        "Socket LGA4710", // 79
        "Socket LGA7529", // 80
    };
    if (this->processorUpgrade < sizeof(processorSocketsList) / sizeof(processorSocketsList[0]) &&
        processorSocketsList[this->processorUpgrade] != nullptr) {
      return processorSocketsList[this->processorUpgrade];
    } else if (this->processorUpgrade == 0xff) {
      return getString(this, this->socketType);
    } else {
      return "unknown";
    }
  }

  const char *PhysicalMemoryArray::toString(char *buf, size_t size) const {
    auto n = ksnprintf(buf, size, "Physical Memory Array handle: %x location: %s use: %s error: %s", this->handle,
                       ::smbios::toString(this->location), ::smbios::toString(this->use),
                       ::smbios::toString(this->memoryErrorCorrection));
    if (this->numberOfDevices) {
      n += ksnprintf(buf + n, size - n, " devices: %d", this->numberOfDevices);
    }
    if (const auto cap = this->getMaximumCapacity(); cap > 0) {
      n += ksnprintf(buf + n, size - n, " max capacity: ");
      n += static_cast<int>(strlen(bytesToHumanReadable(buf + n, size - n, cap)));
    }
    return buf;
  }

  uint64_t PhysicalMemoryArray::getMaximumCapacity() const {
    if (this->maximumCapacity == 0x8000000) {
      if (this->length == sizeof(PhysicalMemoryArray)) {
        return this->extendedMaximumCapacity;
      }
      return 0;
    }
    return this->maximumCapacity * 1024ull;
  }

  const char *MemoryDevice::toString(char *buf, const size_t size) const {
    auto n = ksnprintf(buf, size, "Memory Device %x array %x", this->handle, this->physicalMemoryArrayHandle);
    if (const auto memSize = this->getSize(); memSize > 0) {
      n += ksnprintf(buf + n, size - n, " size: ");
      n += static_cast<int>(strlen(bytesToHumanReadable(buf + n, size - n, memSize)));
    }
    return buf;
  }

  uint64_t MemoryDevice::getSize() const {
    if (this->size == 0x7fff) {
      if (offsetof(configuredMemorySpeed) <= this->length) {
        return (this->extendedSize & ~(1 << 31)) * 1024ull * 1024ull;
      }
      return 0;
    }
    if (this->size & 1 << 15) {
      return (this->size & ~(1 << 15)) * 1024ull;
    }
    return this->size * 1024ull * 1024ull;
  }
} // namespace smbios
