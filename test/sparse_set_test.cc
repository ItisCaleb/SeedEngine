#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "core/container/sparse_set.h"

using namespace Seed;

TEST(SparseSetTest, EmptySet) {
    SparseSet<i32> set;

    EXPECT_EQ(set.size(), 0);
    EXPECT_EQ(set.begin(), set.end());
    EXPECT_EQ(set.get_or_null(0), nullptr);
}

TEST(SparseSetTest, Insert) {
    SparseSet<i32> set;
    i32 first = set.insert(1);
    i32 second = set.insert(2);
    i32 third = set.insert(3);

    ASSERT_NE(set.get_or_null(first), nullptr);
    ASSERT_NE(set.get_or_null(second), nullptr);
    ASSERT_NE(set.get_or_null(third), nullptr);
    EXPECT_EQ(*set.get_or_null(first), 1);
    EXPECT_EQ(*set.get_or_null(second), 2);
    EXPECT_EQ(*set.get_or_null(third), 3);
    EXPECT_EQ(set.size(), 3);
}

TEST(SparseSetTest, InvalidHandleReturnsNull) {
    SparseSet<i32> set;
    set.insert(1);

    EXPECT_EQ(set.get_or_null(-1), nullptr);
    EXPECT_EQ(set.get_or_null(1), nullptr);
    EXPECT_EQ(set.get_or_null(100), nullptr);
}

TEST(SparseSetTest, EraseMiddleElement) {
    SparseSet<i32> set;
    i32 first = set.insert(1);
    i32 erased = set.insert(2);
    i32 third = set.insert(3);

    set.erase(erased);

    ASSERT_NE(set.get_or_null(first), nullptr);
    ASSERT_NE(set.get_or_null(third), nullptr);
    EXPECT_EQ(*set.get_or_null(first), 1);
    EXPECT_EQ(set.get_or_null(erased), nullptr);
    EXPECT_EQ(*set.get_or_null(third), 3);
    EXPECT_EQ(set.size(), 2);
}

TEST(SparseSetTest, EraseFirstElement) {
    SparseSet<i32> set;
    i32 erased = set.insert(1);
    i32 second = set.insert(2);
    i32 third = set.insert(3);

    set.erase(erased);

    EXPECT_EQ(set.get_or_null(erased), nullptr);
    ASSERT_NE(set.get_or_null(second), nullptr);
    ASSERT_NE(set.get_or_null(third), nullptr);
    EXPECT_EQ(*set.get_or_null(second), 2);
    EXPECT_EQ(*set.get_or_null(third), 3);
    EXPECT_EQ(set.size(), 2);
}

TEST(SparseSetTest, EraseLastElement) {
    SparseSet<i32> set;
    i32 first = set.insert(1);
    i32 erased = set.insert(2);

    set.erase(erased);

    ASSERT_NE(set.get_or_null(first), nullptr);
    EXPECT_EQ(*set.get_or_null(first), 1);
    EXPECT_EQ(set.get_or_null(erased), nullptr);
    EXPECT_EQ(set.size(), 1);
}

TEST(SparseSetTest, InvalidEraseDoesNothing) {
    SparseSet<i32> set;
    i32 handle = set.insert(1);

    set.erase(-1);
    set.erase(100);

    ASSERT_NE(set.get_or_null(handle), nullptr);
    EXPECT_EQ(*set.get_or_null(handle), 1);
    EXPECT_EQ(set.size(), 1);
}

TEST(SparseSetTest, RepeatedEraseDoesNothing) {
    SparseSet<i32> set;
    i32 handle = set.insert(1);

    set.erase(handle);
    set.erase(handle);

    EXPECT_EQ(set.get_or_null(handle), nullptr);
    EXPECT_EQ(set.size(), 0);
}

TEST(SparseSetTest, InsertAfterErase) {
    SparseSet<i32> set;
    i32 first = set.insert(1);
    i32 erased = set.insert(2);
    set.erase(erased);

    i32 inserted = set.insert(3);

    ASSERT_NE(set.get_or_null(first), nullptr);
    ASSERT_NE(set.get_or_null(inserted), nullptr);
    EXPECT_EQ(*set.get_or_null(first), 1);
    EXPECT_EQ(*set.get_or_null(inserted), 3);
    EXPECT_EQ(set.size(), 2);
}

TEST(SparseSetTest, IteratorVisitsValues) {
    SparseSet<i32> set;
    set.insert(1);
    set.insert(2);
    set.insert(3);

    std::vector<i32> values(set.begin(), set.end());
    EXPECT_EQ(values, (std::vector<i32>{1, 2, 3}));
}

TEST(SparseSetTest, IteratorCanModifyValues) {
    SparseSet<i32> set;
    i32 first = set.insert(1);
    i32 second = set.insert(2);

    for (i32 &value : set) value *= 2;

    EXPECT_EQ(*set.get_or_null(first), 2);
    EXPECT_EQ(*set.get_or_null(second), 4);
}

TEST(SparseSetTest, ConstIteratorVisitsValues) {
    SparseSet<i32> set;
    set.insert(1);
    set.insert(2);
    const SparseSet<i32> &const_set = set;

    std::vector<i32> values(const_set.begin(), const_set.end());
    EXPECT_EQ(values, (std::vector<i32>{1, 2}));
}

TEST(SparseSetTest, IteratorSkipsErasedValues) {
    SparseSet<i32> set;
    set.insert(1);
    i32 erased = set.insert(2);
    set.insert(3);
    set.erase(erased);

    std::vector<i32> values(set.begin(), set.end());
    EXPECT_EQ(values, (std::vector<i32>{1, 3}));
}

TEST(SparseSetTest, MultipleErasesPreserveRemainingHandles) {
    SparseSet<i32> set;
    std::vector<i32> handles;
    for (i32 value = 0; value < 8; value++) {
        handles.push_back(set.insert(value));
    }

    set.erase(handles[1]);
    set.erase(handles[4]);
    set.erase(handles[6]);

    EXPECT_EQ(set.get_or_null(handles[1]), nullptr);
    EXPECT_EQ(set.get_or_null(handles[4]), nullptr);
    EXPECT_EQ(set.get_or_null(handles[6]), nullptr);
    for (i32 index : {0, 2, 3, 5, 7}) {
        ASSERT_NE(set.get_or_null(handles[(u32)index]), nullptr);
        EXPECT_EQ(*set.get_or_null(handles[(u32)index]), index);
    }

    std::vector<i32> values(set.begin(), set.end());
    std::sort(values.begin(), values.end());
    EXPECT_EQ(values, (std::vector<i32>{0, 2, 3, 5, 7}));
    EXPECT_EQ(set.size(), 5);
}

TEST(SparseSetTest, StoresNonTrivialValues) {
    SparseSet<std::string> set;
    i32 first = set.insert("first");
    i32 second = set.insert("second");
    i32 third = set.insert("third");

    set.erase(second);

    ASSERT_NE(set.get_or_null(first), nullptr);
    ASSERT_NE(set.get_or_null(third), nullptr);
    EXPECT_EQ(*set.get_or_null(first), "first");
    EXPECT_EQ(*set.get_or_null(third), "third");
}
