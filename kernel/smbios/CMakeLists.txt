include(enum2string)
enum2string(HEADER smbios.h TARGET kernel)
cus_target_sources(kernel smbios.cpp smbios.h)
