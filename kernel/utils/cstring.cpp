#include <cstring>
#include <cstdint>

extern "C" {
    size_t strlen(const char *str) {
        size_t len = 0;
        while (str[len]) {
            len++;
        }
        return len;
    }

    char *strncpy(char *dest, const char *src, const size_t n) {
        size_t i = 0;
        while (src[i] && i < n) {
            dest[i] = src[i];
            i++;
        }
        dest[i] = '\0';
        return dest;
    }

    int strcmp(const char *str1, const char *str2) {
        size_t i = 0;
        while (str1[i] && str2[i] && str1[i] == str2[i]) {
            i++;
        }
        return str1[i] - str2[i];
    }

    int strncmp(const char *str1, const char *str2, const size_t n) {
        size_t i = 0;
        while (str1[i] && str2[i] && str1[i] == str2[i] && i < n) {
            i++;
        }
        return str1[i] - str2[i];
    }

    char *strncat(char *dest, const char *src, const size_t n) {
        size_t i = 0;
        while (dest[i]) {
            i++;
        }
        size_t j = 0;
        while (src[j] && j < n) {
            dest[i + j] = src[j];
            j++;
        }
        dest[i + j] = '\0';
        return dest;
    }

    void *memset(void *s, int c, size_t n) {
        auto p = static_cast<uint8_t *>(s);

        for (size_t i = 0; i < n; i++) {
            p[i] = static_cast<uint8_t>(c);
        }

        return s;
    }

    void *memmove(void *dest, const void *src, size_t n) {
        auto *pdest = static_cast<uint8_t *>(dest);
        const auto *psrc = static_cast<const uint8_t *>(src);

        if (src > dest) {
            for (size_t i = 0; i < n; i++) {
                pdest[i] = psrc[i];
            }
        } else if (src < dest) {
            for (size_t i = n; i > 0; i--) {
                pdest[i - 1] = psrc[i - 1];
            }
        }

        return dest;
    }

    int memcmp(const void *s1, const void *s2, size_t n) {
        const auto *p1 = static_cast<const uint8_t *>(s1);
        const auto *p2 = static_cast<const uint8_t *>(s2);

        for (size_t i = 0; i < n; i++) {
            if (p1[i] != p2[i]) {
                return p1[i] < p2[i] ? -1 : 1;
            }
        }

        return 0;
    }
}
