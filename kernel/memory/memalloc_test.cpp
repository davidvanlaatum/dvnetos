#include "memalloc.h"
#include <cstdlib>
#include <gtest/gtest.h>
#include <list>

void *getPage(const size_t) { return nullptr; }

void freePage(void *) {}

class MemPoolTest : public testing::Test {
protected:
  static std::list<void *> pages;
  MemPool *pool = nullptr;

  void SetUp() override {
    pool = new MemPool([](const size_t count) { return pages.emplace_back(malloc(count * PAGE_SIZE)); },
                       [](void *p) {
                         pages.remove_if([p](const void *page) { return page == p; });
                         free(p);
                       });
  }

  void TearDown() override {
    delete pool;
    for (const auto page: pages) {
      free(page);
    }
    pages.clear();
  }
};

std::list<void *> MemPoolTest::pages;

TEST_F(MemPoolTest, SingleAlloc) {
  EXPECT_EQ(pool->usedSize(), 0) << "initial used";
  EXPECT_EQ(pool->freeSize(), 0) << "initial free";
  void *ptr = pool->alloc(10);
  EXPECT_NE(ptr, nullptr);
  EXPECT_EQ(pool->usedSize(), 10) << "used after alloc";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t) * 2 - 10) << "free after alloc";
  pool->free(ptr);
  EXPECT_EQ(pool->usedSize(), 0) << "use after free";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t)) << "free after free";
}

TEST_F(MemPoolTest, MultipleAlloc) {
  EXPECT_EQ(pool->usedSize(), 0) << "initial used";
  EXPECT_EQ(pool->freeSize(), 0) << "initial free";
  EXPECT_EQ(pool->countAlloc(), 0);
  EXPECT_EQ(pool->countFree(), 0);
  void *ptr1 = pool->alloc(10);
  EXPECT_NE(ptr1, nullptr);
  EXPECT_EQ(pool->usedSize(), 10) << "used after 1st alloc";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t) * 2 - 10) << "free after 1st alloc";
  EXPECT_EQ(pool->countAlloc(), 1);
  void *ptr2 = pool->alloc(9);
  EXPECT_NE(ptr2, nullptr);
  EXPECT_NE(ptr1, ptr2);
  EXPECT_EQ(pool->usedSize(), 10 + 9) << "used after 2nd alloc";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t) * 3 - 10 - 9) << "free after 2nd alloc";
  EXPECT_EQ(pool->countAlloc(), 2);
  void *ptr3 = pool->alloc(11);
  EXPECT_NE(ptr3, nullptr);
  EXPECT_NE(ptr3, ptr1);
  EXPECT_NE(ptr3, ptr2);
  EXPECT_EQ(pool->usedSize(), 10 + 9 + 11) << "used after 3rd alloc";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t) * 4 - 10 - 9 - 11) << "free after 3rd alloc";
  EXPECT_EQ(pool->countAlloc(), 3);
  pool->free(ptr3);
  EXPECT_EQ(pool->usedSize(), 10 + 9) << "used after 1st free";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t) * 3 - 10 - 9) << "free after 1st free";
  EXPECT_EQ(pool->countFree(), 1);
  pool->free(ptr1);
  EXPECT_EQ(pool->usedSize(), 9) << "used after 2nd free";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t) * 3 - 9) << "free after 2nd free";
  EXPECT_EQ(pool->countFree(), 2);
  pool->free(ptr2);
  EXPECT_EQ(pool->usedSize(), 0) << "use after 3rd free";
  EXPECT_EQ(pool->freeSize(), PAGE_SIZE - sizeof(size_t)) << "free after 3rd free";
  EXPECT_EQ(pool->countFree(), 3);
}
