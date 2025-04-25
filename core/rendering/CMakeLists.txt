target_sources(seed PRIVATE 
    camera.cc
    material.cc
    model.cc
    terrain.cc
    texture.cc
    )
add_subdirectory(api)
add_subdirectory(renderer)