#ifndef _SEED_EDITOR_STORAGE
#define _SEED_EDITOR_STORAGE
#include "editor/editor_system.h"
#include "core/resource/shader.h"
#include "core/rendering/vertex_layout.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/texture.h"
#include "core/gui/gui.h"

namespace Seed {
class EditorStorage {
    public:
        Ref<Shader> editor_terrain_shader;
        Ref<GuiDocument> editor_ui_doc;

        EditorStorage();
        void reload_shaders();
};

}  // namespace Seed

#endif