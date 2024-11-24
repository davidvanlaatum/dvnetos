#include <cstdarg>
#include <cstddef>
#include <cstdint>

extern "C" {
  int kvsnprintf(char *buffer, const size_t n, const char *format, va_list args) {
    int i = 0, j = 0, total_chars = 0;

    while (format[i] != '\0') {
      if (format[i] == '%' && format[i + 1] != '\0') {
        i++;
        if (format[i] == 'd') {
          int value = va_arg(args, int);
          unsigned char temp[20];
          int k = 0;
          if (value < 0) {
            if (j < n - 1) buffer[j] = '-';
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
            if (j < n - 1) buffer[j] = temp[k]; // NOLINT(*-narrowing-conversions)
            j++;
            total_chars++;
          }
        } else if (format[i] == 's') {
          const char *str = va_arg(args, const char *);
          if (str == nullptr) {
            str = "(null)";
          }
          while (*str != '\0') {
            if (j < n - 1) buffer[j] = *str;
            j++;
            total_chars++;
            str++;
          }
        } else if (format[i] == 'p') {
          const auto ptr = reinterpret_cast<uintptr_t>(va_arg(args, void *));
          unsigned char temp[2 + sizeof(uintptr_t) * 2 + 1]; // "0x" + hex digits + null terminator
          int k = 0;
          temp[k++] = '0';
          temp[k++] = 'x';
          for (int shift = (sizeof(uintptr_t) * 8) - 4; shift >= 0; shift -= 4) {
            const int hex_digit = (ptr >> shift) & 0xF; // NOLINT(*-narrowing-conversions)
            temp[k++] = (hex_digit < 10) ? ('0' + hex_digit) : ('a' + hex_digit - 10);
          }
          temp[k] = '\0';
          for (int l = 0; temp[l] != '\0'; l++) {
            if (j < n - 1) buffer[j] = temp[l]; // NOLINT(*-narrowing-conversions)
            j++;
            total_chars++;
          }
        } else if (format[i] == 'x') {
          unsigned int value = va_arg(args, unsigned int);
          unsigned char temp[20];
          int k = 0;
          do {
            const auto hex_digit = value % 16;
            temp[k++] = (hex_digit < 10) ? ('0' + hex_digit) : ('a' + hex_digit - 10);
            value /= 16;
          } while (value);
          while (k > 0) {
            --k;
            if (j < n - 1) buffer[j] = temp[k]; // NOLINT(*-narrowing-conversions)
            j++;
            total_chars++;
          }
        } else {
          if (j < n - 1) buffer[j] = format[i];
          j++;
          total_chars++;
        }
      } else {
        if (j < n - 1) buffer[j] = format[i];
        j++;
        total_chars++;
      }
      i++;
    }

    if (j < n) {
      buffer[j] = '\0';
    } else if (n > 0) {
      buffer[n - 1] = '\0';
    }

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
}
