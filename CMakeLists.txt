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


add_library(seed STATIC)

target_include_directories(seed PUBLIC .)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)

target_link_libraries(seed PUBLIC 
    glad::glad
    glfw 
    OpenGL::GL
    fmt::fmt
    spdlog::spdlog
    assimp::assimp
)

add_subdirectory(core)

add_executable(app)
target_include_directories(app PRIVATE .)
target_link_libraries(app seed)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")


if(TEST)
    add_subdirectory(test)
else()
    add_subdirectory(game)
endif()