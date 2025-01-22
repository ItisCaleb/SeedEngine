cmake_minimum_required(VERSION 3.14)
project(SeedEngine
    VERSION 0.1
    DESCRIPTION "A Game engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -O3")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

OPTION(TEST
"Build unit test"
OFF)

# The engine static library
add_library(seed STATIC)

target_include_directories(seed PUBLIC .)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)


target_link_libraries(seed PUBLIC 
    glad::glad
    glfw 
    OpenGL::GL
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)
add_subdirectory(core)


# The game code
add_executable(app)
target_include_directories(app PRIVATE .)
target_link_libraries(app seed)
add_subdirectory(game)
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")


if(TEST)
    add_subdirectory(test)
endif()

# The editor
add_executable(editor)
target_include_directories(editor PRIVATE .)
add_subdirectory(editor)

target_link_libraries(editor PUBLIC 
    seed
    assimp::assimp
    nfd::nfd
    imgui::imgui
)