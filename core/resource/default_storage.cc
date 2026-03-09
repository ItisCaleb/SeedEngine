#include "default_storage.h"
#include "core/resource/resource_loader.h"
#include "core/math/vec2.h"

namespace Seed {

DefaultStorage::DefaultStorage() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    mesh_shader = loader->load<Shader>("assets/shader/default.slang");
    skeleton_mesh_shader = loader->load<Shader>("assets/shader/default_skeleton.slang");
    sky_shader = loader->load<Shader>("assets/shader/sky.slang");
    terrain_shader = loader->load<Shader>("assets/shader/terrain.slang");

    post_shader = loader->load<Shader>("assets/shader/post.slang");
    shadow_default_shader =
        loader->load<Shader>("assets/shader/shadow_default.slang");
    shadow_terrain_shader =
        loader->load<Shader>("assets/shader/shadow_terrain.slang");
    billboard_shader = loader->load<Shader>("assets/shader/billboard.slang");
    gui_shader = loader->load<Shader>("assets/shader/imgui.slang");

    shadow_map_default_pipeline =
        RHI::alloc_pipeline(shadow_default_shader->get_handle(),
                            RenderRasterizerState{.cull_mode = Cullmode::FRONT},
                            RenderDepthStencilState{}, {});
    shadow_map_terrain_pipeline =
        RHI::alloc_pipeline(shadow_terrain_shader->get_handle(),
                            RenderRasterizerState{.cull_mode = Cullmode::FRONT,
                                                  .patch_control_points = 4},
                            RenderDepthStencilState{}, {});

    mesh_desc.add_type_attr<Vec3>(0);
    mesh_desc.add_type_attr<Vec3>(1);
    mesh_desc.add_type_attr<Vec3>(2);
    mesh_desc.add_type_attr<Vec2>(3);

    skeleton_mesh_desc.add_type_attr<Vec3>(0);
    skeleton_mesh_desc.add_type_attr<Vec3>(1);
    skeleton_mesh_desc.add_type_attr<Vec3>(2);
    skeleton_mesh_desc.add_type_attr<Vec2>(3);
    skeleton_mesh_desc.add_attr(4, VertexAttributeType::USHORT, 4);
    skeleton_mesh_desc.add_attr(5, VertexAttributeType::FLOAT, 4);

    sky_desc.add_type_attr<Vec3>(0);

    terrain_desc.add_type_attr<Vec2>(0);
    terrain_desc.add_type_attr<Vec2>(1);

    gui_desc.add_type_attr<Vec2>(0);
    gui_desc.add_type_attr<Vec2>(1);
    gui_desc.add_attr(2, VertexAttributeType::UNSIGNED_BYTE, 4, true);

    struct QuadData {
            Vec2 pos;
            Vec2 tex;
    };
    QuadData quad[] = {-1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                       0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                       -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                       1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

    quad_desc.add_type_attr<Vec2>(0);
    quad_desc.add_type_attr<Vec2>(1);
    quad_vertices.create(&quad_desc, (sizeof(quad) / (sizeof(QuadData))), quad);
    u8 white_color[] = {255, 255, 255, 255};
    white_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                         white_color);
    u8 black_color[] = {0, 0, 0, 255};
    black_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                         black_color);
}
}  // namespace Seed