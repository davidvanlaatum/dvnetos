#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#include <gtest/gtest.h>
#define DEBUG_PRINT(x) GTEST_LOG_(INFO) << x
#else
#define DEBUG_PRINT(x)
#endif

#endif //DEBUG_H
