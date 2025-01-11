add_executable(app 
    game.cc)
target_include_directories(app PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(app seed)

set(EXECUTABLE_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/build")

