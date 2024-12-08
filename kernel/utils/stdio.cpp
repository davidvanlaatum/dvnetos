#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#ifdef __KERNEL__
#include "panic.h"
#endif

struct formatSpec {
  char type;
  char padding;
  char lengthMod[2];
  bool leftJustify;
  bool forceSign;
  bool spaceForSign;
  bool altForm;
  int minLength;
  int precision;
};

template<typename T = int>
int formatDecimal(char *buffer, const size_t n, const formatSpec format, T value, int &total_chars) {
  int j = 0;
  unsigned char temp[20];
  int k = 0;
  if (std::is_signed_v<T> && value < 0) {
    if (j < n - 1)
      buffer[j] = '-';
    j++;
    total_chars++;
    value = -value;
  }
  do {
    temp[k++] = '0' + (value % 10);
    value /= 10;
  } while (value);
  while (k > 0) {
    --k;
    if (j < n - 1)
      buffer[j] = temp[k]; // NOLINT(*-narrowing-conversions)
    j++;
    total_chars++;
  }
  return j;
}

int formatHex(char *buffer, const size_t n, const formatSpec format, uint64_t value, int &total_chars) {
  unsigned char temp[20];
  int k = 0, j = 0;
  do {
    const auto hex_digit = value % 16;
    temp[k++] = (hex_digit < 10) ? ('0' + hex_digit) : ((format.type == 'X' ? 'A' : 'a') + hex_digit - 10);
    value /= 16;
  } while (value);
  while (k > 0) {
    --k;
    if (j < n - 1)
      buffer[j] = temp[k]; // NOLINT(*-narrowing-conversions)
    j++;
    total_chars++;
  }
  return j;
}

int formatOct(char *buffer, const size_t n, const formatSpec format, uint64_t value, int &total_chars) {
  unsigned char temp[20];
  int k = 0, j = 0;
  do {
    temp[k++] = '0' + (value % 8);
    value /= 8;
  } while (value);
  while (k > 0) {
    --k;
    if (j < n - 1)
      buffer[j] = temp[k]; // NOLINT(*-narrowing-conversions)
    j++;
    total_chars++;
  }
  return j;
}

int formatString(char *buffer, const size_t n, const formatSpec spec, const char *ptr, int &total_chars) {
  int j = 0;
  if (ptr == nullptr) {
    ptr = "(null)";
  }
  while (*ptr != '\0') {
    if (j < n - 1)
      buffer[j] = *ptr;
    j++;
    total_chars++;
    ptr++;
  }
  return j;
}

int formatChar(char *buffer, const size_t n, const formatSpec spec, int c, int &total_chars) {
  if (n > 0)
    buffer[0] = c; // NOLINT(*-narrowing-conversions)
  total_chars++;
  return 1;
}

int formatPointer(char *buffer, const size_t n, const formatSpec spec, const void *ptr, int &total_chars) {
  unsigned char temp[2 + sizeof(uintptr_t) * 2 + 1]; // "0x" + hex digits + null terminator
  int k = 0, j = 0;
  temp[k++] = '0';
  temp[k++] = 'x';
  for (int shift = (sizeof(uintptr_t) * 8) - 4; shift >= 0; shift -= 4) {
    const int hex_digit = (reinterpret_cast<uintptr_t>(ptr) >> shift) & 0xF; // NOLINT(*-narrowing-conversions)
    temp[k++] = (hex_digit < 10) ? ('0' + hex_digit) : ('a' + hex_digit - 10);
  }
  temp[k] = '\0';
  for (int l = 0; temp[l] != '\0'; l++) {
    if (j < n - 1)
      buffer[j] = temp[l]; // NOLINT(*-narrowing-conversions)
    j++;
    total_chars++;
  }
  return j;
}

int kvsnprintf(char *buffer, const size_t n, const char *format, va_list args) { // NOLINT(*-non-const-parameter)
  int i = 0, j = 0, total_chars = 0;

  while (format[i] != '\0') {
#define rem (j >= n ? 0 : (n - j))
    if (format[i] == '%' && format[i + 1] != '\0') {
      i++;
      if (format[i] == '%') {
        if (j < n - 1)
          buffer[j] = '%';
        j++;
        total_chars++;
        i++;
        continue;
      }
      formatSpec spec = {.padding = ' '};
      if (format[i] == '0') {
        spec.padding = '0';
        i++;
      }
      if (format[i] == '+') {
        spec.forceSign = true;
        i++;
      }
      if (format[i] == '-') {
        spec.leftJustify = true;
        i++;
      }
      while (format[i] >= '0' && format[i] <= '9') {
        spec.minLength = spec.minLength * 10 + (format[i] - '0');
        i++;
      }
      if (format[i] == '.') {
        i++;
        spec.precision = 0;
        while (format[i] >= '0' && format[i] <= '9') {
          spec.precision = spec.precision * 10 + (format[i] - '0');
          i++;
        }
      }
      if (format[i] == 'l' || format[i] == 'h') {
        spec.lengthMod[0] = format[i];
        i++;
        if (format[i] == 'l' || format[i] == 'h') {
          spec.lengthMod[1] = format[i];
          i++;
        }
      }
      spec.type = format[i];
      i++;
      if (spec.type == 'd' || spec.type == 'i') {
        if (spec.lengthMod[0] == 'l' && spec.lengthMod[1] == 'l') {
          j += formatDecimal(buffer + j, rem, spec, va_arg(args, long long), total_chars);
        } else if (spec.lengthMod[0] == 'l') {
          j += formatDecimal(buffer + j, rem, spec, va_arg(args, long), total_chars);
        } else {
          j += formatDecimal(buffer + j, rem, spec, va_arg(args, int), total_chars);
        }
      } else if (spec.type == 'x' || spec.type == 'X' || spec.type == 'o' || spec.type == 'u') {
        unsigned long long value;
        if (spec.lengthMod[0] == 'l' && spec.lengthMod[1] == 'l') {
          value = va_arg(args, unsigned long long);
        } else if (spec.lengthMod[0] == 'l') { // NOLINT(*-branch-clone)
          value = va_arg(args, unsigned long);
        } else {
          value = va_arg(args, unsigned int);
        }
        if (spec.type == 'u') {
          j += formatDecimal(buffer + j, rem, spec, value, total_chars);
        } else if (spec.type == 'o') {
          j += formatOct(buffer + j, rem, spec, value, total_chars);
        } else {
          j += formatHex(buffer + j, rem, spec, value, total_chars);
        }
      } else if (spec.type == 's') {
        j += formatString(buffer + j, rem, spec, va_arg(args, const char *), total_chars);
      } else if (spec.type == 'c') {
        j += formatChar(buffer + j, rem, spec, va_arg(args, int), total_chars);
      } else if (spec.type == 'p') {
        j += formatPointer(buffer + j, rem, spec, va_arg(args, void *), total_chars);
#ifdef __KERNEL__
      } else {
        kpanicf("unsupported format specifier: %c", spec.type);
#endif
      }
    } else {
      if (rem > 1)
        buffer[j] = format[i];
      j++;
      i++;
      total_chars++;
    }
  }

  if (j < n) {
    buffer[j] = '\0';
  } else if (n > 0) {
    buffer[n - 1] = '\0';
  }
#undef rem

  return total_chars;
}

int ksnprintf(char *buffer, const size_t n, const char *format, ...) {
  va_list args;
  va_start(args, format);
  int result = kvsnprintf(buffer, n, format, args);
  if (result >= n) {
    result = n - 1; // NOLINT(*-narrowing-conversions)
  }
  va_end(args);
  return result;
}
