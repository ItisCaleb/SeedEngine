#include "editor_storage.h"
#include "core/resource/resource_loader.h"

namespace Seed {

EditorStorage::EditorStorage() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    editor_terrain_shader = DS::get_instance()->terrain_shader->create_variant(
        {ShaderDefine{.name = "EDITOR", .value = "1"}});
    editor_ui_doc =
        loader->load_internal<GuiDocument>("assets/editor/ui/editor.rml");
}

void EditorStorage::reload_shaders() {
    editor_terrain_shader->reload_from_disk();
}
}  // namespace Seed