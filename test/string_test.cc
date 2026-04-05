#include <fmt/base.h>
#include <gtest/gtest.h>
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

TEST(KStrTest, SplitAt) {
    KStr haystack = "hello world!";
    KStr t1 = "he";

    KStr t2 = "llo world!";
    auto split = haystack.split_at(2);
    EXPECT_EQ(split.first, t1);
    EXPECT_EQ(split.second, t2);
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


TEST(KStrTest, Pop) {
    KStr src = "歡迎來到 Ave Mujica 的世界";
    KStr target = "歡迎來到 Ave Mujica";
    KString s(src);
    s.pop(4);
    EXPECT_EQ(s.to_str(), target);
}
