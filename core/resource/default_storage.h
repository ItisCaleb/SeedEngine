#ifndef _SEED_DEFAULT_STORAGE
#define _SEED_DEFAULT_STORAGE
#include "core/resource/shader.h"
#include "core/rendering/vertex_layout.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/texture.h"

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
        Ref<Shader> post_shader;
        Ref<Shader> decal_shader;
        Ref<Shader> shadow_default_shader;
        Ref<Shader> shadow_terrain_shader;
        Ref<Texture> default_texture;
        VertexLayout terrain_desc;
        VertexLayout mesh_desc;
        VertexLayout sky_desc;
        VertexLayout gui_desc;
        VertexLayout post_desc;
        Ref<VertexData> post_data;
        RenderResource shadow_map_default_pipeline;
        RenderResource shadow_map_terrain_pipeline;

        static DefaultStorage *get_instance() { return instance; }
        DefaultStorage();

};

}  // namespace Seed

#endif