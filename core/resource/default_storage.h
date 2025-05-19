#ifndef _SEED_DEFAULT_STORAGE
#define _SEED_DEFAULT_STORAGE
#include "core/resource/shader.h"
#include "core/rendering/vertex_desc.h"

namespace Seed {
#define DS DefaultStorage

class DefaultStorage {
    private:
        inline static DefaultStorage *instance = nullptr;
        Ref<Shader> sky_shader;
        Ref<Shader> mesh_shader;
        Ref<Shader> gui_shader;
        Ref<Shader> terrain_shader;
        Ref<Shader> mesh_debug_shader;
        VertexDescription terrain_desc;
        VertexDescription mesh_desc;
        VertexDescription sky_desc;
        VertexDescription gui_desc;

    public:
        static DefaultStorage *get_instance() { return instance; }
        DefaultStorage();
        Ref<Shader> get_sky_shader() { return sky_shader; }
        Ref<Shader> get_mesh_shader() { return mesh_shader; }
        Ref<Shader> get_mesh_debug_shader() { return mesh_debug_shader; }

        Ref<Shader> get_gui_shader() { return gui_shader; }
        Ref<Shader> get_terrain_shader() { return terrain_shader; }
        VertexDescription *get_terrain_desc() { return &terrain_desc; }
        VertexDescription *get_mesh_desc() { return &mesh_desc; }
        VertexDescription *get_sky_desc() { return &sky_desc; }
        VertexDescription *get_gui_desc() { return &gui_desc; }
};

}  // namespace Seed

#endif