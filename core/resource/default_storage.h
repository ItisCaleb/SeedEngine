#ifndef _SEED_DEFAULT_STORAGE
#define _SEED_DEFAULT_STORAGE
#include "core/resource/shader.h"
#include "core/rendering/vertex_layout.h"
#include "core/rendering/vertex_data.h"

namespace Seed {
#define DS DefaultStorage

class DefaultStorage {
    inline static DefaultStorage *instance = nullptr;
    public:
        Ref<Shader> sky_shader;
        Ref<Shader> mesh_shader;
        Ref<Shader> gui_shader;
        Ref<Shader> terrain_shader;
        Ref<Shader> debug_shader;
        Ref<Shader> mesh_debug_shader;
        Ref<Shader> post_shader;
        Ref<Shader> decal_shader;
        VertexLayout terrain_desc;
        VertexLayout mesh_desc;
        VertexLayout sky_desc;
        VertexLayout gui_desc;
        VertexLayout post_desc;
        VertexData post_data;

        static DefaultStorage *get_instance() { return instance; }
        DefaultStorage();

};

}  // namespace Seed

#endif