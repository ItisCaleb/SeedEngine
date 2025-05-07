target_sources(seed PRIVATE 
    engine.cc
    input.cc
    world.cc
    entity.cc
    model_entity.cc
    input_handler.cc
    os.cc
    )


add_subdirectory(rendering)
add_subdirectory(allocator)
add_subdirectory(container)
add_subdirectory(collision)
add_subdirectory(resource)
add_subdirectory(concurrency)