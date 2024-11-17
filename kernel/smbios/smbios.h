#ifndef SMBIOS_H
#define SMBIOS_H

#include <cstdint>

namespace smbios {
  struct Entry32 {
    char anchor[4];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint16_t max_structure_size;
    uint8_t implementation_revision;
    char formatted_area[5];
    char anchor_string[5];
    uint8_t checksum2;
    uint16_t table_length;
    uint32_t table_address;
    uint16_t number_of_structures;
    uint8_t bcd_revision;
  } __attribute__((packed));

  struct Entry64 {
    char anchor[5];
    uint8_t checksum;
    uint8_t length;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t docrev;
    uint8_t entry_point_revision;
    uint8_t reserved;
    uint32_t table_length;
    uint64_t table_address;
  } __attribute__((packed));

  enum class TableType : uint8_t {
    FirmwareInfo = 0,
    SystemInfo = 1,
    BaseboardInfo = 2,
    ChassisInfo = 3,
    ProcessorInfo = 4,
    MemoryControllerInfo = 5,
    MemoryModuleInfo = 6,
    CacheInfo = 7,
    PortConnectorInfo = 8,
    SystemSlotsInfo = 9,
    OnBoardDevicesInfo = 10,
    OEMStrings = 11,
    SystemConfigurationOptions = 12,
    BIOSLanguageInfo = 13,
    GroupAssociations = 14,
    SystemEventLog = 15,
    PhysicalMemoryArray = 16,
    MemoryDevice = 17,
    MemoryErrorInfo32 = 18,
    MemoryArrayMappedAddress = 19,
    MemoryDeviceMappedAddress = 20,
    BuiltinPointingDevice = 21,
    PortableBattery = 22,
    SystemReset = 23,
    HardwareSecurity = 24,
    SystemPowerControls = 25,
    VoltageProbe = 26,
    CoolingDevice = 27,
    TemperatureProbe = 28,
    ElectricalCurrentProbe = 29,
    OutOfBandRemoteAccess = 30,
    BootIntegrityServices = 31,
    SystemBootInfo = 32,
    MemoryErrorInfo64 = 33,
    ManagementDevice = 34,
    ManagementDeviceComponent = 35,
    ManagementDeviceThresholdData = 36,
    MemoryChannel = 37,
    IPMIDeviceInfo = 38,
    SystemPowerSupply = 39,
    AdditionalInfo = 40,
    OnboardDevicesExtInfo = 41,
    ManagementControllerHostInterface = 42,
    TPMDevice = 43,
    ProcessorAdditionalInfo = 44,
    MemoryModuleAdditionalInfo = 45,
    ManagementComponentTransport = 46,
    Inactive = 126,
    EndOfTable = 127
  };

  struct TableHeader {
    TableType type;
    uint8_t length;
    uint16_t handle;
  } __attribute__((packed));

  enum class FirmwareCharacteristics : uint64_t {
    Unknown = 1 << 2,
    BIOSCharacteristicsNotSupported = 1 << 3,
    ISA = 1 << 4,
    MCA = 1 << 5,
    EISA = 1 << 6,
    PCI = 1 << 7,
    PCMCIA = 1 << 8,
    PlugAndPlay = 1 << 9,
    APM = 1 << 10,
    BIOSIsUpgradable = 1 << 11,
    BIOSShadowingIsAllowed = 1 << 12,
    VLVISupported = 1 << 13,
    ESCD = 1 << 14,
    BootFromCD = 1 << 15,
    SelectableBoot = 1 << 16,
    BIOSROMIsSocketed = 1 << 17,
    BootFromPCMCIA = 1 << 18,
    EDDSpecification = 1 << 19,
    JapaneseNecFloppy = 1 << 20,
    JapaneseToshibaFloppy = 1 << 21,
    Floppy5251_2 = 1 << 22,
    Floppy35_2 = 1 << 23,
    ATAPIZIPDriveBoot = 1 << 24,
    Boot1394 = 1 << 25,
    SmartBattery = 1 << 26,
    BIOSBootSpecification = 1 << 27,
    FunctionKeyInitiatedNetworkBoot = 1 << 28,
    TargetedContentDistribution = 1 << 29,
    UEFISpecification = 1 << 30,
    SMBIOSSpecification = 1UL << 31,
  };

  // Type 0
  struct FirmwareInfo : TableHeader {
    uint8_t vendor;
    uint8_t version;
    uint16_t biosStartAddress;
    uint8_t releaseDate;
    uint8_t romSize;
    uint64_t characteristics;
    uint8_t characteristics_ext[2];
    uint8_t systemBiosMajorRelease;
    uint8_t systemBiosMinorRelease;
    uint8_t ecFirmwareMajorRelease;
    uint8_t ecFirmwareMinorRelease;
    uint16_t extendedFirmwareRomSize;

    const char *toString(char *buf, size_t size) const;
  } __attribute__((packed));

  static_assert(sizeof(FirmwareInfo::characteristics) == 8);

  // Type 1
  struct SystemInfo : TableHeader {
    uint8_t vendor;
    uint8_t product;
    uint8_t version;
    uint8_t serial;
    uint8_t uuid[16];
    uint8_t wakeUpType;
    uint8_t sku;
    uint8_t family;

    const char *toString(char *buf, size_t size) const;
  } __attribute__((packed));

  enum class ChassisState : uint8_t {
    Other = 1,
    Unknown = 2,
    Safe = 3,
    Warning = 4,
    Critical = 5,
    NonRecoverable = 6
  };

  enum class ChassisSecurityStatus : uint8_t {
    Other = 1,
    Unknown = 2,
    None = 3,
    ExternalInterfaceLockedOut = 4,
    ExternalInterfaceEnabled = 5
  };

  // Type 3
  struct ChassisInfo : TableHeader {
    uint8_t manufacturer;
    uint8_t type;
    uint8_t version;
    uint8_t serial;
    uint8_t assetTag;
    ChassisState bootUpState;
    ChassisState powerSupplyState;
    ChassisState thermalState;
    ChassisSecurityStatus securityStatus;
    uint32_t oemDefined;
    uint8_t height;
    uint8_t numberOfPowerCords;
    uint8_t containedElementCount;
    uint8_t containedElementRecordLength;

    const char *toString(char *buf, size_t size) const;
  } __attribute__((packed));

  enum class ProcessorType : uint8_t {
    Other = 1,
    Unknown = 2,
    CentralProcessor = 3,
    MathProcessor = 4,
    DSPProcessor = 5,
    VideoProcessor = 6
  };

  enum class ProcessorFamily : uint8_t {
    SeeProcessorFamily2 = 0xFE,
  };

  enum class ProcessorStatus : uint8_t {
    Unknown = 0,
    CPUEnabled = 1,
    CPUDisabledByUser = 2,
    CPUDisabledByBIOS = 3,
    CPUIdle = 4,
    Other = 7,
  };

  // Type 4
  struct ProcessorInfo : TableHeader {
    uint8_t socketDesignation;
    ProcessorType processorType;
    ProcessorFamily processorFamily;
    uint8_t processorManufacturer;
    uint64_t processorId;
    uint8_t processorVersion;
    uint8_t voltage;
    uint16_t externalClock;
    uint16_t maxSpeed;
    uint16_t currentSpeed;
    uint8_t status;
    uint8_t processorUpgrade;
    uint16_t l1CacheHandle;
    uint16_t l2CacheHandle;
    uint16_t l3CacheHandle;
    uint8_t serialNumber;
    uint8_t assetTag;
    uint8_t partNumber;
    uint8_t coreCount;
    uint8_t coreEnabled;
    uint8_t threadCount;
    uint16_t processorCharacteristics;
    uint16_t processorFamily2;
    uint16_t coreCount2;
    uint16_t coreEnabled2;
    uint16_t threadCount2;
    uint16_t threadEnabled;
    uint8_t socketType;

    const char *toString(char *buf, size_t size) const;

    [[nodiscard]] const char *getDesignation() const;

    [[nodiscard]] ProcessorType getType() const;

    [[nodiscard]] ProcessorFamily getFamily() const;

    [[nodiscard]] const char *getManufacturer() const;

    [[nodiscard]] uint64_t getId() const;

    [[nodiscard]] const char *getVersion() const;

    [[nodiscard]] uint16_t getExternalClock() const;

    [[nodiscard]] uint16_t getMaxSpeed() const;

    [[nodiscard]] uint16_t getCurrentSpeed() const;

    [[nodiscard]] bool isPopulated() const;

    [[nodiscard]] ProcessorStatus getStatus() const;

    [[nodiscard]] const char *getStatusString() const;

    [[nodiscard]] const char *getSocket() const;

    [[nodiscard]] const char *getSerialNumber() const;

    [[nodiscard]] const char *getAssetTag() const;

    [[nodiscard]] const char *getPartNumber() const;

    [[nodiscard]] uint8_t getCores() const;

    [[nodiscard]] uint8_t getCoresEnabled() const;

    [[nodiscard]] uint8_t getThreads() const;
  } __attribute__((packed));

  enum class PhysicalMemoryArrayLocation : uint8_t {
    Other = 1,
    Unknown = 2,
    SystemBoardOrMotherboard = 3,
    ISAAddOnCard = 4,
    EISAAddOnCard = 5,
    PCIAddOnCard = 6,
    MCAAddOnCard = 7,
    PCMCIAAddOnCard = 8,
    ProprietaryAddOnCard = 9,
    NuBus = 10,
    PC98C20 = 160,
    PC98C24 = 161,
    PC98E = 162,
    PC98LocalBus = 163,
  };

  enum class PhysicalMemoryArrayUse : uint8_t {
    Other = 1,
    Unknown = 2,
    SystemMemory = 3,
    VideoMemory = 4,
    FlashMemory = 5,
    NonVolatileRAM = 6,
    CacheMemory = 7,
  };

  enum class MemoryErrorCorrection : uint8_t {
    Other = 1,
    Unknown = 2,
    None = 3,
    Parity = 4,
    SingleBitECC = 5, // STR:"Single-bit error correction"
    MultiBitECC = 6,  // STR:"Multi-bit error correction"
    CRC = 7,
  };

  // Type 16
  struct PhysicalMemoryArray : TableHeader {
    PhysicalMemoryArrayLocation location;
    PhysicalMemoryArrayUse use;
    MemoryErrorCorrection memoryErrorCorrection;
    uint32_t maximumCapacity;
    uint16_t errorHandle;
    uint16_t numberOfDevices;
    uint64_t extendedMaximumCapacity;

    const char *toString(char *buf, size_t size) const;

    [[nodiscard]] uint64_t getMaximumCapacity() const;
  } __attribute__((packed));

  // Type 17
  struct MemoryDevice : TableHeader {
    uint16_t physicalMemoryArrayHandle;
    uint16_t memoryErrorInformationHandle;
    uint16_t totalWidth;
    uint16_t dataWidth;
    uint16_t size;
    uint8_t formFactor;
    uint8_t deviceSet;
    uint8_t deviceLocator;
    uint8_t bankLocator;
    uint8_t memoryType;
    uint16_t typeDetail;
    uint16_t speed;
    uint8_t manufacturer;
    uint8_t serialNumber;
    uint8_t assetTag;
    uint8_t partNumber;
    uint8_t attributes;
    uint32_t extendedSize;
    uint16_t configuredMemorySpeed;
    uint16_t minimumVoltage;
    uint16_t maximumVoltage;
    uint16_t configuredVoltage;
    uint8_t memoryTechnology;
    uint16_t memoryOperatingModeCapability;
    uint8_t firmwareVersion;
    uint16_t moduleManufacturer;
    uint16_t moduleProduct;
    uint16_t memorySubSystemControllerManufacturerId;
    uint16_t memorySubSystemControllerProductId;
    uint64_t nonVolatileSize;
    uint64_t volatileSize;
    uint64_t cacheSize;
    uint64_t logicalSize;
    uint32_t extendedSpeed;
    uint16_t extendedConfiguredMemorySpeed;
    uint16_t pmic0ManufacturerId;
    uint16_t pmic0RevisionNumber;
    uint16_t rcdManufacturerId;
    uint16_t rcdRevisionNumber;

    [[nodiscard]] uint64_t getSize() const;

    const char *toString(char *buf, size_t size) const;
  } __attribute__((packed));

  static_assert(sizeof(MemoryDevice) == 0x62);

  class SMBIOS {
  public:
    void init(uint64_t hhdmOffset);
    void dumpTable(const TableHeader *table, uint16_t length);
  };

  extern SMBIOS defaultSMBIOS;
}

#endif //SMBIOS_H
