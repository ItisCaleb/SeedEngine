target_sources(seed PRIVATE 
    camera.cc
    vertex_layout.cc
    vertex_data.cc
    shadow_map.cc
    instance_data.cc
    mesh_storage.cc
    viewport.cc
    light.cc
)
add_subdirectory(rhi)
add_subdirectory(renderer)
add_subdirectory(backend)