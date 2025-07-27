target_sources(seed PRIVATE 
    camera.cc
    vertex_layout.cc
    vertex_data.cc
    render_target.cc
    )
add_subdirectory(api)
add_subdirectory(renderer)