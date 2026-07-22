#include "editor_storage.h"
#include "core/resource/default_storage.h"
#include "core/resource/resource_loader.h"
#include "core/system.h"

namespace Seed {

EditorStorage::EditorStorage() {
    ResourceLoader *loader = System::gResourceLoader;
    editor_terrain_shader =
        System::gDefaultStorage->terrain_shader->create_variant(
            {ShaderDefine{.name = "EDITOR", .value = "1"}});
    editor_ui_doc =
        loader->load_internal<GuiDocument>("assets/editor/ui/editor.rml");
}

void EditorStorage::reload_shaders() {
    editor_terrain_shader->reload_from_disk();
}
}  // namespace Seed
