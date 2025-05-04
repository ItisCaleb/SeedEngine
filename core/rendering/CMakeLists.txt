target_sources(seed PRIVATE 
    camera.cc
    vertex_desc.cc
    vertex_data.cc
    )
add_subdirectory(api)
add_subdirectory(renderer)