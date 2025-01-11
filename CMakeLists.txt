cmake_minimum_required(VERSION 3.14)
project(SeedEngine
    VERSION 0.1
    DESCRIPTION "A Game engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -O3")

OPTION(TEST
"Build unit test"
OFF)

add_subdirectory(src)

if(TEST)
    add_subdirectory(test)
else()
    add_subdirectory(game)
endif()