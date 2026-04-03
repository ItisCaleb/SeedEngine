#include <fmt/base.h>
#include <gtest/gtest.h>
#include <string_view>
#include <vector>
#include "core/container/kstring.h"

using namespace Seed;

TEST(KStrTest, StartWith) {
    KStr haystack = "hello world";
    KStr needle = "hello";
    EXPECT_TRUE(haystack.start_with(needle));
    std::string_view v;
}

TEST(KStrTest, EndWith) {
    KStr haystack = "hello world!";
    KStr needle = "world!";
    EXPECT_TRUE(haystack.end_with(needle));
}

TEST(KStrTest, Contains) {
    KStr haystack = "hello world!";
    KStr needle = "o w";
    EXPECT_TRUE(haystack.contains(needle));
}

TEST(KStrTest, FindFirst) {
    KStr haystack = "hello world";
    KStr needle = "world";
    EXPECT_EQ(haystack.find_first(needle), 6);
}

TEST(KStrTest, FindFirstNotFound) {
    KStr haystack = "hello";
    KStr needle = "world";
    EXPECT_EQ(haystack.find_first(needle), -1);
}

TEST(KStrTest, UTF8) {
    KStr haystack = "你好世界";
    KStr needle = "世界";
    EXPECT_EQ(haystack.find_first(needle), 6);  // byte offset
}

TEST(KStrTest, SplitPath) {
    KStr haystack = "/Users/abc/你好/我是/abc";
    KStr needle = "/";
    std::vector<KStr> targets = {"", "Users", "abc", "你好", "我是", "abc"};
    auto splits = haystack.split(needle);
    EXPECT_EQ(splits.size(), targets.size());
    for (u32 i = 0; i < splits.size(); i++) {
        EXPECT_EQ(splits[i], targets[i]);
    }
}

TEST(KStrTest, SplitMulti) {
    KStr haystack = "ab::cd::ef::gh";
    KStr needle = "::";
    std::vector<KStr> targets = {"ab", "cd", "ef", "gh"};
    auto splits = haystack.split(needle);
    EXPECT_EQ(splits.size(), targets.size());
    for (u32 i = 0; i < splits.size(); i++) {
        EXPECT_EQ(splits[i], targets[i]);
    }
}

TEST(KStrTest, Trim) {
    KStr src = "\t\n\t hello! \t\n\t";
    KStr target = "hello!";
    EXPECT_EQ(src.trim(), target);
}

TEST(KStrTest, Replace) {
    KStr src = "C://a/b/c/d.txt";
    KStr target = "C:\\\\a\\b\\c\\d.txt";
    EXPECT_EQ(src.replace("/", "\\").to_str(), target);
}
