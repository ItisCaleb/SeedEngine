#include "default_storage.h"
#include "core/resource/resource_loader.h"

namespace Seed {
DefaultStorage::DefaultStorage() {
    instance = this;
    ResourceLoader *loader = ResourceLoader::get_instance();
    mesh_shader =
        loader->load_shader("assets/default.vert", "assets/default.frag");
    sky_shader = loader->load_shader("assets/sky.vert", "assets/sky.frag");
    terrain_shader =
        loader->load_shader("assets/terrain.vert", "assets/terrain.frag", "",
                            "assets/terrain.tesc", "assets/terrain.tese");

    mesh_debug_shader =
        loader->load_shader("assets/mesh_debug.vert", "assets/mesh_debug.frag");
    const char *vertex_shader =
        "#version 410 core\n"
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
        "#version 410 core\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "uniform sampler2D u_texture[8];\n"
        "layout (location = 0) out vec4 Out_Color;\n"
        "void main()\n"
        "{\n"
        "    Out_Color = Frag_Color * texture(u_texture[0], Frag_UV.st);\n"
        "}\n";

    gui_shader.create(vertex_shader, fragment_shader);

    mesh_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    mesh_desc.add_attr(1, VertexAttributeType::FLOAT, 3, 0);
    mesh_desc.add_attr(2, VertexAttributeType::FLOAT, 2, 0);

    sky_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);

    terrain_desc.add_attr(0, VertexAttributeType::FLOAT, 2, 0);
    terrain_desc.add_attr(1, VertexAttributeType::FLOAT, 2, 0);

    gui_desc.add_attr(0, VertexAttributeType::FLOAT, 2, 0);
    gui_desc.add_attr(1, VertexAttributeType::FLOAT, 2, 0);
    gui_desc.add_attr(2, VertexAttributeType::UNSIGNED_BYTE, 4, 0, true);


}
}  // namespace Seed