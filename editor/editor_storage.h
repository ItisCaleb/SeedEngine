#ifndef _SEED_EDITOR_STORAGE
#define _SEED_EDITOR_STORAGE
#include "core/resource/shader.h"
#include "core/rendering/vertex_layout.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/texture.h"
#include "core/gui/gui.h"

namespace Seed {
#define ES EditorStorage

class EditorStorage {
        inline static EditorStorage *instance = nullptr;

    public:
        Ref<Shader> editor_terrain_shader;
        Ref<GuiDocument> editor_ui_doc;

        static EditorStorage *get_instance() { return instance; }
        EditorStorage();
        void reload_shaders();
};

}  // namespace Seed

#endif