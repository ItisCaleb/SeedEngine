target_sources(seed PRIVATE 
    engine.cc
    input.cc
    window.cc
    input_handler.cc
    project.cc
    os.cc
)


add_subdirectory(rendering)
add_subdirectory(allocator)
add_subdirectory(container)
add_subdirectory(collision)
add_subdirectory(resource)
add_subdirectory(concurrency)
add_subdirectory(gui)
add_subdirectory(debug)
add_subdirectory(physic)
add_subdirectory(misc)
add_subdirectory(io)

if(BUILD_XR)
    add_subdirectory(xr)
endif()
add_subdirectory(world)
