add_library(seed STATIC 
    engine.cc
    input.cc
    render_engine.cc
    resource.cc
    shader.cc
    )

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)


target_include_directories(seed PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(seed PUBLIC 
    glad::glad
    glfw 
    OpenGL::GL
    fmt::fmt
    spdlog::spdlog
    assimp::assimp
)

