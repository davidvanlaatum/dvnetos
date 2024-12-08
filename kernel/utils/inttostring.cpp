#include "inttostring.h"

void intToString(int value, char *str, const int base) {
  char *ptr = str;
  char *ptr1 = str;

  // Handle negative numbers for base 10
  if (value < 0 && base == 10) {
    *ptr++ = '-';
    value = -value;
  }

  // Convert the number to the given base
  do {
    const auto tmp_value = value;
    value /= base;
    *ptr++ = "0123456789abcdef"[tmp_value - value * base];
  } while (value);

  // Null-terminate the string
  *ptr-- = '\0';

  // Reverse the string
  while (ptr1 < ptr) {
    const auto tmp_char = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp_char;
  }
}
