target_sources(editor PRIVATE 
    editor.cc 
    resource.cc
    editor_gui.cc
    editor_storage.cc
)

add_subdirectory(project)
add_subdirectory(terrain)