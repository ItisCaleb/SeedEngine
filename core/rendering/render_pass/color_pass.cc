#include "color_pass.h"
#include "core/rendering/light.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource.h"
#include <spdlog/spdlog.h>
#include <vector>

namespace Seed {
void ColorPass::init() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    try {
        this->shader =
            loader->loadShader("assets/vertex.glsl", "assets/fragment.glsl");
    } catch (std::exception &e) {
        spdlog::error("Error loading Shader: {}", e.what());
        exit(1);
    }
    RenderResource::register_resource("Default", this->shader);
    std::vector<VertexAttribute> vertices_attrs = {
        {.layout_num = 0, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 1, .size = 3, .stride = sizeof(Vertex)},
        {.layout_num = 2, .size = 2, .stride = sizeof(Vertex)}};
    std::vector<VertexAttribute> instance_attrs = {
        {
            .layout_num = 3,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 4,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 5,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
        {
            .layout_num = 6,
            .is_instance = true,
            .size = 4,
            .stride = sizeof(Mat4),
        },
    };
    vertices_desc_rc.alloc_vertex_desc(vertices_attrs);
    instance_desc_rc.alloc_vertex_desc(instance_attrs);
    u8 white_tex[4] = {255, 255, 255, 255};
    RenderResource default_tex;
    default_tex.alloc_texture(1, 1, white_tex);
    default_material = Material::create(default_tex, default_tex, default_tex);

    RenderResource lights_rc;

    Lights lights;
    lights.ambient = Vec3{0.2, 0.2, 0.2};
    lights.lights[0].set_position(Vec3{2, 0, 2});
    lights.lights[0].diffuse = Vec3{0.9, 0.5, 0.5};
    lights.lights[0].specular = Vec3{1, 1, 1};

    lights.lights[1].set_direction(Vec3{2, 3, -1});
    lights.lights[1].diffuse = Vec3{1, 1, 1};
    lights_rc.alloc_constant(sizeof(Lights), &lights);

    RenderResource::register_resource("Lights", lights_rc);
}
void ColorPass::process(RenderCommandDispatcher &dp, u64 sort_key) {
    for (auto &[model, instances] : model_instances) {
        dp.begin(sort_key++);
        dp.use(&model->instance_rc);
        dp.use(&this->instance_desc_rc);

        for (auto &[mat_variant, matrices] : instances) {
            dp.begin(sort_key++);
            dp.update(&model->instance_rc, 0, sizeof(Mat4) * matrices.size(),
                      (void *)matrices.data());
            for (Mesh &mesh : model->meshes) {
                dp.use(&mesh.vertices_rc);
                dp.use(&this->vertices_desc_rc);
                dp.use(&mesh.indices_rc);
                Ref<Material> material = model->model_mat.get_material(
                    mesh.material_handle, mat_variant);
                if (material.is_null()) {
                    material = default_material;
                }
                if (material->diffuse_map.inited()) {
                    dp.use(&material->diffuse_map, 0);
                } else {
                    dp.use(&default_material->diffuse_map, 0);
                }
                if (material->specular_map.inited()) {
                    dp.use(&material->specular_map, 1);
                } else {
                    dp.use(&default_material->specular_map, 1);
                }
                dp.render(&this->shader, RenderPrimitiveType::TRIANGLES,
                          matrices.size());
            }
            dp.end();
        }
        dp.end();
    }

}
}  // namespace Seed