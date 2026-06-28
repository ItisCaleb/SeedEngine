#include "default_storage.h"
#include "core/resource/resource_loader.h"
#include "core/math/vec2.h"

namespace Seed {

Vec3 skyboxVertices[] = {
    // positions
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

DefaultStorage::DefaultStorage() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    mesh_shader = loader->load_internal<Shader>("assets/shader/default.slang");
    skeleton_mesh_shader = mesh_shader->create_variant({{"BONE", "1"}});
    sky_shader = loader->load_internal<Shader>("assets/shader/sky.slang");
    terrain_shader =
        loader->load_internal<Shader>("assets/shader/terrain.slang");

    post_shader = loader->load_internal<Shader>("assets/shader/post.slang");
    billboard_shader =
        loader->load_internal<Shader>("assets/shader/billboard.slang");
    gui_shader = loader->load_internal<Shader>("assets/shader/imgui.slang");
    rml_shader = loader->load_internal<Shader>("assets/shader/rmlui.slang");
    noise_texture = loader->load_internal<Texture>("assets/noise.png");
    noise_texture->update_sampler(SamplerProperty{
        .wrap_u = SamplerWrap::REPEAT, .wrap_v = SamplerWrap::REPEAT});

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

    gui_desc.add_type_attr<Vec2>(0);
    gui_desc.add_type_attr<Vec2>(1);
    gui_desc.add_attr(2, VertexAttributeType::UNSIGNED_BYTE, 4, true);

    struct QuadData {
            Vec2 pos;
            Vec2 tex;
    };
    /* uv origin is top-left */
    QuadData quad[] = {-1.0f, 1.0f, 0.0f, 0.0f,  -1.0f, -1.0f,
                       0.0f,  1.0f, 1.0f, -1.0f, 1.0f,  1.0f,

                       -1.0f, 1.0f, 0.0f, 0.0f,  1.0f,  -1.0f,
                       1.0f,  1.0f, 1.0f, 1.0f,  1.0f,  0.0f};

    quad_desc.add_type_attr<Vec2>(0);
    quad_desc.add_type_attr<Vec2>(1);
    quad_vertices.create(&quad_desc, (sizeof(quad) / (sizeof(QuadData))), quad);

    sky_vertices.create(&DS::get_instance()->sky_desc,
                        (sizeof(skyboxVertices) / sizeof(Vec3)), skyboxVertices,
                        UpdateFrequence::STATIC);
    u8 white_color[] = {255, 255, 255, 255};
    white_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                         white_color);
    u8 black_color[] = {0, 0, 0, 255};
    black_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                         black_color);
    u8 normal_color[] = {128, 128, 255, 255};
    normal_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                          normal_color);
}

void DefaultStorage::reload_shaders() {
    mesh_shader->reload_from_disk();
    skeleton_mesh_shader->reload_from_disk();
    sky_shader->reload_from_disk();
    terrain_shader->reload_from_disk();
    post_shader->reload_from_disk();
    billboard_shader->reload_from_disk();
    gui_shader->reload_from_disk();
    rml_shader->reload_from_disk();
    // debug_shader->reload_from_disk();
    // decal_shader->reload_from_disk();
}
}  // namespace Seed