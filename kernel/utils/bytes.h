#ifndef BYTES_H
#define BYTES_H
#include <cstddef>
#include <cstdint>

const char *bytesToHumanReadable(char *buf, size_t len, uint64_t bytes);

#endif // BYTES_H
