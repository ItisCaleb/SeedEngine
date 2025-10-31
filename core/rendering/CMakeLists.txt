target_sources(seed PRIVATE 
    camera.cc
    vertex_layout.cc
    vertex_data.cc
    render_target.cc
    shadow_map.cc
    instance_data.cc
    mesh_storage.cc
    viewport.cc
    light.cc
)
add_subdirectory(api)
add_subdirectory(renderer)