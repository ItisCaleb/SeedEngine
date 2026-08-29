#include <gtest/gtest.h>

#include "core/concurrency/fiber.h"
#include <stdio.h>

using namespace Seed;

void test_print(Fiber *fiber){
    printf("hello world!\n");
}

void test_yield_print(Fiber *fiber){
    printf("before yield!\n");
    fiber->yield();
    printf("after yield!\n");
}

TEST(FiberTest, TestResume) {
    Fiber fiber(test_print);

    printf("Start fiber\n");
    fiber.resume();
    printf("End fiber\n");
}

TEST(FiberTest, TestYield) {
    Fiber fiber(test_yield_print);

    printf("Start fiber\n");
    fiber.resume();
    printf("yield\n");
    fiber.resume();
    printf("End fiber\n");
}