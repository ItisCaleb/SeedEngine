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
        Ref<Shader> skeleton_mesh_shader;

        Ref<Shader> gui_shader;
        Ref<Shader> terrain_shader;
        Ref<Shader> debug_shader;
        Ref<Shader> post_shader;
        Ref<Shader> decal_shader;
        Ref<Shader> shadow_default_shader;
        Ref<Shader> shadow_terrain_shader;
        Ref<Shader> billboard_shader;
        Ref<Texture> white_texture;
        Ref<Texture> black_texture;

        VertexLayout terrain_desc;
        VertexLayout mesh_desc;
        VertexLayout skeleton_mesh_desc;
        VertexLayout sky_desc;
        VertexLayout gui_desc;
        VertexLayout quad_desc;
        Ref<VertexData> quad_vertices;
        PipelineHandle shadow_map_default_pipeline;
        PipelineHandle shadow_map_terrain_pipeline;

        static DefaultStorage *get_instance() { return instance; }
        DefaultStorage();
};

}  // namespace Seed

#endif