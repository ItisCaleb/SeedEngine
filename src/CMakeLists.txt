add_library(seed STATIC 
    main.cc)

find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
target_include_directories(seed PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(seed PUBLIC glfw)
target_link_libraries(seed PRIVATE OpenGL::GL)

