#include <cstddef>
#include <cstdint>
#include <cstdio>

const char *bytesToHumanReadable(char *buf, const size_t len, const uint64_t bytes) {
  auto llen = 0;
  uint64_t remainder;
  if (bytes < 1024) {
    ksnprintf(buf, len, "%lu B", bytes);
    remainder = 0;
  } else if (bytes < 1024 * 1024) {
    llen = ksnprintf(buf, len, "%lu KiB", bytes / 1024);
    remainder = bytes % 1024;
  } else if (bytes < 1024 * 1024 * 1024) {
    llen = ksnprintf(buf, len, "%lu MiB", bytes / 1024 / 1024);
    remainder = bytes % (1024 * 1024);
  } else {
    llen = ksnprintf(buf, len, "%lu GiB", bytes / 1024 / 1024 / 1024);
    remainder = bytes % (1024 * 1024 * 1024);
  }
  if (remainder > 0 && llen < len) {
    buf[llen] = ' ';
    llen++;
    bytesToHumanReadable(buf + llen, len - llen, remainder);
  }
  return buf;
}
