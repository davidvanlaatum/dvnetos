#include <cstdio>
#include "/Users/david/CLionProjects/dvnetos/kernel/smbios/smbios.h"

namespace smbios {
const char *toString(TableType value) {
    switch (value) {
        case TableType::FirmwareInfo: return "FirmwareInfo";
        case TableType::SystemInfo: return "SystemInfo";
        case TableType::BaseboardInfo: return "BaseboardInfo";
        case TableType::ChassisInfo: return "ChassisInfo";
        case TableType::ProcessorInfo: return "ProcessorInfo";
        case TableType::MemoryControllerInfo: return "MemoryControllerInfo";
        case TableType::MemoryModuleInfo: return "MemoryModuleInfo";
        case TableType::CacheInfo: return "CacheInfo";
        case TableType::PortConnectorInfo: return "PortConnectorInfo";
        case TableType::SystemSlotsInfo: return "SystemSlotsInfo";
        case TableType::OnBoardDevicesInfo: return "OnBoardDevicesInfo";
        case TableType::OEMStrings: return "OEMStrings";
        case TableType::SystemConfigurationOptions: return "SystemConfigurationOptions";
        case TableType::BIOSLanguageInfo: return "BIOSLanguageInfo";
        case TableType::GroupAssociations: return "GroupAssociations";
        case TableType::SystemEventLog: return "SystemEventLog";
        case TableType::PhysicalMemoryArray: return "PhysicalMemoryArray";
        case TableType::MemoryDevice: return "MemoryDevice";
        case TableType::MemoryErrorInfo32: return "MemoryErrorInfo32";
        case TableType::MemoryArrayMappedAddress: return "MemoryArrayMappedAddress";
        case TableType::MemoryDeviceMappedAddress: return "MemoryDeviceMappedAddress";
        case TableType::BuiltinPointingDevice: return "BuiltinPointingDevice";
        case TableType::PortableBattery: return "PortableBattery";
        case TableType::SystemReset: return "SystemReset";
        case TableType::HardwareSecurity: return "HardwareSecurity";
        case TableType::SystemPowerControls: return "SystemPowerControls";
        case TableType::VoltageProbe: return "VoltageProbe";
        case TableType::CoolingDevice: return "CoolingDevice";
        case TableType::TemperatureProbe: return "TemperatureProbe";
        case TableType::ElectricalCurrentProbe: return "ElectricalCurrentProbe";
        case TableType::OutOfBandRemoteAccess: return "OutOfBandRemoteAccess";
        case TableType::BootIntegrityServices: return "BootIntegrityServices";
        case TableType::SystemBootInfo: return "SystemBootInfo";
        case TableType::MemoryErrorInfo64: return "MemoryErrorInfo64";
        case TableType::ManagementDevice: return "ManagementDevice";
        case TableType::ManagementDeviceComponent: return "ManagementDeviceComponent";
        case TableType::ManagementDeviceThresholdData: return "ManagementDeviceThresholdData";
        case TableType::MemoryChannel: return "MemoryChannel";
        case TableType::IPMIDeviceInfo: return "IPMIDeviceInfo";
        case TableType::SystemPowerSupply: return "SystemPowerSupply";
        case TableType::AdditionalInfo: return "AdditionalInfo";
        case TableType::OnboardDevicesExtInfo: return "OnboardDevicesExtInfo";
        case TableType::ManagementControllerHostInterface: return "ManagementControllerHostInterface";
        case TableType::TPMDevice: return "TPMDevice";
        case TableType::ProcessorAdditionalInfo: return "ProcessorAdditionalInfo";
        case TableType::MemoryModuleAdditionalInfo: return "MemoryModuleAdditionalInfo";
        case TableType::ManagementComponentTransport: return "ManagementComponentTransport";
        case TableType::Inactive: return "Inactive";
        case TableType::EndOfTable: return "EndOfTable";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(FirmwareCharacteristics value) {
    switch (value) {
        case FirmwareCharacteristics::Unknown: return "Unknown";
        case FirmwareCharacteristics::BIOSCharacteristicsNotSupported: return "BIOSCharacteristicsNotSupported";
        case FirmwareCharacteristics::ISA: return "ISA";
        case FirmwareCharacteristics::MCA: return "MCA";
        case FirmwareCharacteristics::EISA: return "EISA";
        case FirmwareCharacteristics::PCI: return "PCI";
        case FirmwareCharacteristics::PCMCIA: return "PCMCIA";
        case FirmwareCharacteristics::PlugAndPlay: return "PlugAndPlay";
        case FirmwareCharacteristics::APM: return "APM";
        case FirmwareCharacteristics::BIOSIsUpgradable: return "BIOSIsUpgradable";
        case FirmwareCharacteristics::BIOSShadowingIsAllowed: return "BIOSShadowingIsAllowed";
        case FirmwareCharacteristics::VLVISupported: return "VLVISupported";
        case FirmwareCharacteristics::ESCD: return "ESCD";
        case FirmwareCharacteristics::BootFromCD: return "BootFromCD";
        case FirmwareCharacteristics::SelectableBoot: return "SelectableBoot";
        case FirmwareCharacteristics::BIOSROMIsSocketed: return "BIOSROMIsSocketed";
        case FirmwareCharacteristics::BootFromPCMCIA: return "BootFromPCMCIA";
        case FirmwareCharacteristics::EDDSpecification: return "EDDSpecification";
        case FirmwareCharacteristics::JapaneseNecFloppy: return "JapaneseNecFloppy";
        case FirmwareCharacteristics::JapaneseToshibaFloppy: return "JapaneseToshibaFloppy";
        case FirmwareCharacteristics::Floppy5251_2: return "Floppy5251_2";
        case FirmwareCharacteristics::Floppy35_2: return "Floppy35_2";
        case FirmwareCharacteristics::ATAPIZIPDriveBoot: return "ATAPIZIPDriveBoot";
        case FirmwareCharacteristics::Boot1394: return "Boot1394";
        case FirmwareCharacteristics::SmartBattery: return "SmartBattery";
        case FirmwareCharacteristics::BIOSBootSpecification: return "BIOSBootSpecification";
        case FirmwareCharacteristics::FunctionKeyInitiatedNetworkBoot: return "FunctionKeyInitiatedNetworkBoot";
        case FirmwareCharacteristics::TargetedContentDistribution: return "TargetedContentDistribution";
        case FirmwareCharacteristics::UEFISpecification: return "UEFISpecification";
        case FirmwareCharacteristics::SMBIOSSpecification: return "SMBIOSSpecification";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(ChassisState value) {
    switch (value) {
        case ChassisState::Other: return "Other";
        case ChassisState::Unknown: return "Unknown";
        case ChassisState::Safe: return "Safe";
        case ChassisState::Warning: return "Warning";
        case ChassisState::Critical: return "Critical";
        case ChassisState::NonRecoverable: return "NonRecoverable";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(ChassisSecurityStatus value) {
    switch (value) {
        case ChassisSecurityStatus::Other: return "Other";
        case ChassisSecurityStatus::Unknown: return "Unknown";
        case ChassisSecurityStatus::None: return "None";
        case ChassisSecurityStatus::ExternalInterfaceLockedOut: return "ExternalInterfaceLockedOut";
        case ChassisSecurityStatus::ExternalInterfaceEnabled: return "ExternalInterfaceEnabled";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(ProcessorType value) {
    switch (value) {
        case ProcessorType::Other: return "Other";
        case ProcessorType::Unknown: return "Unknown";
        case ProcessorType::CentralProcessor: return "CentralProcessor";
        case ProcessorType::MathProcessor: return "MathProcessor";
        case ProcessorType::DSPProcessor: return "DSPProcessor";
        case ProcessorType::VideoProcessor: return "VideoProcessor";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(ProcessorFamily value) {
    switch (value) {
        case ProcessorFamily::SeeProcessorFamily2: return "SeeProcessorFamily2";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(ProcessorStatus value) {
    switch (value) {
        case ProcessorStatus::Unknown: return "Unknown";
        case ProcessorStatus::CPUEnabled: return "CPUEnabled";
        case ProcessorStatus::CPUDisabledByUser: return "CPUDisabledByUser";
        case ProcessorStatus::CPUDisabledByBIOS: return "CPUDisabledByBIOS";
        case ProcessorStatus::CPUIdle: return "CPUIdle";
        case ProcessorStatus::Other: return "Other";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(PhysicalMemoryArrayLocation value) {
    switch (value) {
        case PhysicalMemoryArrayLocation::Other: return "Other";
        case PhysicalMemoryArrayLocation::Unknown: return "Unknown";
        case PhysicalMemoryArrayLocation::SystemBoardOrMotherboard: return "SystemBoardOrMotherboard";
        case PhysicalMemoryArrayLocation::ISAAddOnCard: return "ISAAddOnCard";
        case PhysicalMemoryArrayLocation::EISAAddOnCard: return "EISAAddOnCard";
        case PhysicalMemoryArrayLocation::PCIAddOnCard: return "PCIAddOnCard";
        case PhysicalMemoryArrayLocation::MCAAddOnCard: return "MCAAddOnCard";
        case PhysicalMemoryArrayLocation::PCMCIAAddOnCard: return "PCMCIAAddOnCard";
        case PhysicalMemoryArrayLocation::ProprietaryAddOnCard: return "ProprietaryAddOnCard";
        case PhysicalMemoryArrayLocation::NuBus: return "NuBus";
        case PhysicalMemoryArrayLocation::PC98C20: return "PC98C20";
        case PhysicalMemoryArrayLocation::PC98C24: return "PC98C24";
        case PhysicalMemoryArrayLocation::PC98E: return "PC98E";
        case PhysicalMemoryArrayLocation::PC98LocalBus: return "PC98LocalBus";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(PhysicalMemoryArrayUse value) {
    switch (value) {
        case PhysicalMemoryArrayUse::Other: return "Other";
        case PhysicalMemoryArrayUse::Unknown: return "Unknown";
        case PhysicalMemoryArrayUse::SystemMemory: return "SystemMemory";
        case PhysicalMemoryArrayUse::VideoMemory: return "VideoMemory";
        case PhysicalMemoryArrayUse::FlashMemory: return "FlashMemory";
        case PhysicalMemoryArrayUse::NonVolatileRAM: return "NonVolatileRAM";
        case PhysicalMemoryArrayUse::CacheMemory: return "CacheMemory";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

const char *toString(MemoryErrorCorrection value) {
    switch (value) {
        case MemoryErrorCorrection::Other: return "Other";
        case MemoryErrorCorrection::Unknown: return "Unknown";
        case MemoryErrorCorrection::None: return "None";
        case MemoryErrorCorrection::Parity: return "Parity";
        case MemoryErrorCorrection::SingleBitECC: return "Single-bit error correction";
        case MemoryErrorCorrection::MultiBitECC: return "Multi-bit error correction";
        case MemoryErrorCorrection::CRC: return "CRC";
        default: {
            static char buffer[32];
            ksnprintf(buffer, sizeof(buffer), "Unknown(%d)", static_cast<int>(value));
            return buffer;
        }
    }
}

}
