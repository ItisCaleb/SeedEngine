cmake_minimum_required(VERSION 3.14)
project(SeedEngine
    VERSION 0.1
    DESCRIPTION "A Game engine"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)

OPTION(TEST
"Build unit test"
OFF)

# add_compile_options("-fsanitize=address")
# add_link_options("-fsanitize=address")
# add_compile_definitions(
#     _DISABLE_STRING_ANNOTATION=1
#     _DISABLE_VECTOR_ANNOTATION=1
# )

# The engine static library
add_library(seed STATIC)

target_include_directories(seed PUBLIC .)


include(FetchContent)


find_package(glfw3 CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(nfd CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(volk CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(OpenXR CONFIG REQUIRED)
set_target_properties(Jolt::Jolt PROPERTIES CPP_RTTI_ENABLED ON)

target_link_libraries(seed PUBLIC 
    glfw 
    Vulkan::Vulkan
    volk::volk
    volk::volk_headers
    fmt::fmt
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    Jolt::Jolt
    imgui::imgui
    nfd::nfd
    slang::slang
    GPUOpen::VulkanMemoryAllocator
)


if(BUILD_XR)
    add_library(seed_xr STATIC)
    target_link_libraries(seed_xr PUBLIC 
        OpenXR::headers
        OpenXR::openxr_loader)
    target_include_directories(seed_xr PUBLIC .)
    target_link_libraries(seed_xr PUBLIC seed)
    target_compile_definitions(seed PRIVATE SEED_XR)
endif()
target_compile_definitions(seed PRIVATE
        $<$<CONFIG:Debug>:VULKAN_DEBUG>

)



add_subdirectory(core)
add_subdirectory(thirdparty)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set_source_files_properties(
    "core/resource/image.cc"
    PROPERTIES
    COMPILE_OPTIONS "-mavx2;-mavx512vl;-mavx512bw"
)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    # ARM 不需要這些指令，可以留空或加入 -march=armv8-a
    message(STATUS "Detected ARM architecture, disabling AVX flags")
endif()


# The game code
add_executable(app)
target_include_directories(app PRIVATE .)
target_link_libraries(app seed)
if(BUILD_XR)
    target_link_libraries(app seed_xr)
endif()
add_subdirectory(game)
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/bin")


set_target_properties(app PROPERTIES
    DEBUGGER_WORKING_DIRECTORY  "${PROJECT_SOURCE_DIR}"
)
if(TEST)
    FetchContent_Declare(
    googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(googletest)
    target_compile_options(gtest PRIVATE -Wno-character-conversion)
    target_compile_options(gtest_main PRIVATE -Wno-character-conversion)

    add_executable(tests)
    add_subdirectory(test)

    target_link_libraries(tests
        GTest::gtest_main
        seed
    )

    include(GoogleTest)
    gtest_discover_tests(tests)
endif()

# The editor
add_executable(editor)
target_include_directories(editor PRIVATE .)
add_subdirectory(editor)

target_link_libraries(editor PUBLIC 
    seed
    assimp::assimp
)
