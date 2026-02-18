#include "editor_storage.h"
#include "core/resource/resource_loader.h"
#include "core/math/vec2.h"

namespace Seed {

EditorStorage::EditorStorage() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    editor_terrain_shader = loader->load<Shader>("assets/shader/editor_terrain.slang");
}
}  // namespace Seed