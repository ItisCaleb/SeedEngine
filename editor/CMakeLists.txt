target_sources(editor PRIVATE 
    editor.cc 
    editor_gui.cc
    editor_storage.cc
    camera_entity.cc
)

add_subdirectory(project)
add_subdirectory(world)
add_subdirectory(asset)
add_subdirectory(gui)