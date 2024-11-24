#include <gtest/gtest.h>

#include "include/stdio.h"

TEST(stdio, ksnprintf) {
  char buf[256];
  int n = ksnprintf(buf, sizeof(buf), "abc");
  EXPECT_STREQ(buf, "abc");
  EXPECT_EQ(n, 3);
  n = ksnprintf(buf, sizeof(buf), "abc%d", 123);
  EXPECT_STREQ(buf, "abc123");
  EXPECT_EQ(n, 6);
  n = ksnprintf(buf, sizeof(buf), "abc%d%s", 123, "def");
  EXPECT_STREQ(buf, "abc123def");
  EXPECT_EQ(n, 9);
  n = ksnprintf(buf, sizeof(buf), "abc%p", 456);
  EXPECT_STREQ(buf, "abc0x00000000000001c8");
  EXPECT_EQ(n, 21);
}

TEST(stdio, ksnprintfOverflow) {
  char buf[5];
  const int n = ksnprintf(buf, sizeof(buf), "abcdefg");
  EXPECT_STREQ(buf, "abcd");
  EXPECT_EQ(n, 4);
}
