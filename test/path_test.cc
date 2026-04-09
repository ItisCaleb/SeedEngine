#include <fmt/base.h>
#include <gtest/gtest.h>
#include <vector>
#include "core/io/path.h"

using namespace Seed;

TEST(PathTest, Filename) {
    KStr src = "/a/b/c.png";
    KStr target = "c.png";
    Path p(src);
    EXPECT_EQ(p.filename(), target);
}

TEST(PathTest, Extension) {
    KStr src = "/a/b/c.png";
    KStr target = ".png";
    Path p(src);
    EXPECT_EQ(p.extension(), target);
}

TEST(PathTest, Push) {
    Path p("/");
    p.push("a");
    EXPECT_EQ(p.to_str(), "\\a");
    p.push("b");
    EXPECT_EQ(p.to_str(), "\\a\\b");
    p.push("c");
    EXPECT_EQ(p.to_str(), "\\a\\b\\c");
}

TEST(PathTest, Pop) {
    KStr src = "/a/b/c.png";
    KStr target = ".png";
    Path p(src);
    p.pop();
    EXPECT_EQ(p.to_str(), "\\a\\b");
    p.pop();
    EXPECT_EQ(p.to_str(), "\\a");
    p.pop();
    EXPECT_EQ(p.to_str(), "\\");
}

TEST(PathTest, Normalize) {
    KStr src = "/a///../b/././c";
    Path p(src);
    p.normalize();
    EXPECT_EQ(p.to_str(), "\\b\\c");
}

TEST(PathTest, Absolute) {
    KStr src = "C:\\a\\..\\b\\.\\c";
    Path p(src);
    p.absolute();
    EXPECT_EQ(p.to_str(), "C:\\b\\c");
}

TEST(PathTest, Canonicalize) {
    KStr src = "./";
    Path p(src);
    EXPECT_TRUE(p.canonicalize());
    fmt::println("{}", p.to_str());
}