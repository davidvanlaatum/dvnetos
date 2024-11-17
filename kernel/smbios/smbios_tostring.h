#include "smbios.h"

namespace smbios {
const char *toString(TableType value);
const char *toString(FirmwareCharacteristics value);
const char *toString(ChassisState value);
const char *toString(ChassisSecurityStatus value);
const char *toString(ProcessorType value);
const char *toString(ProcessorFamily value);
const char *toString(ProcessorStatus value);
const char *toString(PhysicalMemoryArrayLocation value);
const char *toString(PhysicalMemoryArrayUse value);
const char *toString(MemoryErrorCorrection value);
}
