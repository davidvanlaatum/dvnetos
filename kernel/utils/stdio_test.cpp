#include <gtest/gtest.h>

#include "include/stdio.h"

TEST(stdio, ksnprintf) {
    char buf[256];
    ksnprintf(buf, sizeof(buf), "abc");
    EXPECT_STREQ(buf, "abc");
    ksnprintf(buf, sizeof(buf), "abc%d", 123);
    EXPECT_STREQ(buf, "abc123");
    ksnprintf(buf, sizeof(buf), "abc%d%s", 123, "def");
    EXPECT_STREQ(buf, "abc123def");
    ksnprintf(buf, sizeof(buf), "abc%p", 456);
    EXPECT_STREQ(buf, "abc0x00000000000001c8");
}
