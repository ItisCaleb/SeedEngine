#include "default_storage.h"
#include "core/resource/resource_loader.h"
#include "core/math/vec2.h"

namespace Seed {

struct PostData {
        Vec2 pos;
        Vec2 tex;
};
DefaultStorage::DefaultStorage() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    mesh_shader = loader->load_shader("assets/shader/default.vert",
                                      "assets/shader/default.frag");
    sky_shader =
        loader->load_shader("assets/shader/sky.vert", "assets/shader/sky.frag");
    terrain_shader = loader->load_shader(
        "assets/shader/terrain.vert", "assets/shader/terrain.frag", "",
        "assets/shader/terrain.tesc", "assets/shader/terrain.tese");

    post_shader = loader->load_shader("assets/shader/post.vert",
                                      "assets/shader/post.frag");
    shadow_default_shader = loader->load_shader(
        "assets/shader/shadow_default.vert", "assets/shader/shadow.frag");
    shadow_terrain_shader = loader->load_shader(
        "assets/shader/shadow_terrain.vert", "assets/shader/shadow.frag", "",
        "assets/shader/shadow_terrain.tesc",
        "assets/shader/shadow_terrain.tese");
    billboard_shader = loader->load_shader("assets/shader/billboard.vert", "assets/shader/billboard.frag");

    shadow_map_default_pipeline.alloc_pipeline(
        shadow_default_shader->get_render_resource(),
        RenderRasterizerState{.cull_mode = Cullmode::FRONT},
        RenderDepthStencilState{.depth_on = true}, {});
    shadow_map_terrain_pipeline.alloc_pipeline(
        shadow_terrain_shader->get_render_resource(),
        RenderRasterizerState{.cull_mode = Cullmode::FRONT,
                              .patch_control_points = 4},
        RenderDepthStencilState{.depth_on = true}, {});

    const char *vertex_shader =
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 UV;\n"
        "layout (location = 2) in vec4 Color;\n"
        "layout (std140) uniform GUIProjMtx {\n"
        "   mat4 ProjMtx;\n"
        "};\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main()\n"
        "{\n"
        "    Frag_UV = UV;\n"
        "    Frag_Color = Color;\n"
        "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
        "}\n";

    const char *fragment_shader =
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D u_texture[8];\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(u_texture[0], Frag_UV.st);\n"
        "}\n";

    gui_shader.create(vertex_shader, fragment_shader);
    mesh_desc.add_type_attr<Vec3>(0, 0);
    mesh_desc.add_type_attr<Vec3>(1, 0);
    mesh_desc.add_type_attr<Vec3>(2, 0);
    mesh_desc.add_type_attr<Vec2>(3, 0);

    sky_desc.add_type_attr<Vec3>(0, 0);

    terrain_desc.add_type_attr<Vec2>(0, 0);
    terrain_desc.add_type_attr<Vec2>(1, 0);

    gui_desc.add_type_attr<Vec2>(0, 0);
    gui_desc.add_type_attr<Vec2>(1, 0);
    gui_desc.add_attr(2, VertexAttributeType::UNSIGNED_BYTE, 4, 0, true);

    PostData tmp_post[] = {-1.0f, 1.0f, 0.0f, 1.0f,  -1.0f, -1.0f,
                           0.0f,  0.0f, 1.0f, -1.0f, 1.0f,  0.0f,

                           -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  -1.0f,
                           1.0f,  0.0f, 1.0f, 1.0f,  1.0f,  1.0f};

    quad_desc.add_type_attr<Vec2>(0, 0);
    quad_desc.add_type_attr<Vec2>(1, 0);
    quad_vertices.create(&quad_desc, (sizeof(tmp_post) / (sizeof(PostData))),
                     tmp_post);
    u8 white_color[] = {255, 255, 255, 255};
    white_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                         white_color);
    u8 black_color[] = {0, 0, 0, 255};
    black_texture.create(TextureType::TEXTURE_2D, 1, 1, PixelFormat::RGBA,
                         black_color);
}
}  // namespace Seed