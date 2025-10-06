#include "default_renderer.h"
#include "core/rendering/light.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/resource_loader.h"
#include "core/engine.h"
#include <spdlog/spdlog.h>
#include <vector>
#include "core/debug/debug_drawer.h"

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

void DefaultRenderer::init_color() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    instance_desc.add_attr(3, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(4, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(5, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(6, VertexAttributeType::FLOAT, 4, 1);

    debug_mat.create(DS::get_instance()->mesh_debug_shader);
    RenderRasterizerState rst = {.poly_mode = PolygonMode::LINE};
    debug_mat->set_rasterizer_state(rst);

    u_lights.alloc_constant("Lights", sizeof(STB140Lights), nullptr);
    auto terrain_model = Mat4::translate_mat({0, 0, 0}).transpose();
    terrain_m.alloc_constant("TerrainMatrices", sizeof(Mat4), &terrain_model);
}

void DefaultRenderer::init() {
    init_color();

    sky_vert.alloc_vertex(sizeof(Vec3), (sizeof(skyboxVertices) / sizeof(Vec3)),
                          skyboxVertices);
}

void DefaultRenderer::preprocess() {
    World *world = SeedEngine::get_instance()->get_world();

    std::vector<ModelEntity *> &entities = world->get_model_entities();
    Camera *cam = RenderEngine::get_instance()->get_cam();
    for (ModelEntity *e : entities) {
        Ref<Model> model = e->get_model();
        if (model.is_null()) continue;
        AABB bounding_box = e->get_model_aabb();
        /* frustum culling */
        if (cam && !cam->within_frustum(bounding_box)) {
            continue;
        }
        auto &instance = model_instances[*model];
        instance.push_back(e->get_transform().transpose());
        entity_aabb.push_back(bounding_box);
    }

    DebugDrawer *drawer = DebugDrawer::get_instance();
    debug_line.alloc_vertex(sizeof(DebugDrawer::DebugVertex),
                            drawer->line_vertices.size(),
                            drawer->line_vertices.data());
    debug_triangle.alloc_vertex(sizeof(DebugDrawer::DebugVertex),
                                drawer->triangle_vertices.size(),
                                drawer->triangle_vertices.data());
    debug_triangle.alloc_index(drawer->triangle_indices);
}

void DefaultRenderer::process(Viewport &viewport) {
    World *world = SeedEngine::get_instance()->get_world();

    RenderCommandDispatcher dp;
    dp.begin_scope("Default Rendering", current_sort_key());

    STB140Lights *light_buf = (STB140Lights *)dp.map_buffer(
        u_lights, 0, sizeof(STB140Lights), current_sort_key());
    light_buf->u_dir_light = world->get_direction_light().get_stb140();
    light_buf->u_light_ambient = world->get_ambient_light();
    for (u32 i = 0;
         i < (sizeof(light_buf->u_point_lights) / sizeof(STB140Light)); i++) {
        if (i < world->get_point_lights().size()) {
            light_buf->u_point_lights[i] =
                world->get_point_lights()[i].get_stb140();

        } else {
            light_buf->u_point_lights[i].enable = 0.0f;
        }
    }

    for (auto &[model, instances] : model_instances) {
        if (instances.empty()) {
            continue;
        }
        dp.update_buffer(model->instance_rc, 0, sizeof(Mat4) * instances.size(),
                         (void *)instances.data());
        for (Ref<Mesh> mesh : model->meshes) {
            RenderDrawDataBuilder mesh_builder = dp.generate_render_data(
                ref_cast<Material>(mesh->get_material()));
            mesh_builder.bind_vertex_data(mesh->vertex_data);
            mesh_builder.bind_description(&DS::get_instance()->mesh_desc);
            mesh_builder.bind_vertex(model->instance_rc);
            mesh_builder.bind_description(&instance_desc);

            dp.render(mesh_builder, RenderPrimitiveType::TRIANGLES,
                      mesh->get_material()->get_pipeline(),
                      current_sort_key(0.1));
        }
    }

    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    if (terrain.is_valid()) {
        RenderDrawDataBuilder builder = dp.generate_render_data(
            ref_cast<Material>(terrain->get_material()));
        builder.bind_vertex_data(*terrain->get_vertices(), 0);
        builder.bind_description(&DS::get_instance()->terrain_desc);

        dp.render(builder, RenderPrimitiveType::PATCHES,
                  terrain->get_material()->get_pipeline(), current_sort_key(1));
    }

    auto sky = SeedEngine::get_instance()->get_world()->get_sky();
    if (sky.is_valid()) {
        RenderDrawDataBuilder sky_builder =
            dp.generate_render_data(ref_cast<Material>(sky->get_material()));
        sky_builder.bind_vertex_data(sky_vert);
        sky_builder.bind_description(&DS::get_instance()->sky_desc);
        dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
                  sky->get_material()->get_pipeline(), current_sort_key(1.0));
    }
    dp.end_scope(next_sort_key());

    // shadow mapping

    {
        DebugDrawer *drawer = DebugDrawer::get_instance();
        if (drawer->try_lock()) {
            RenderDrawDataBuilder line_builder =
                dp.generate_render_data(drawer->debug_mat);
            line_builder.bind_vertex_data(debug_line);
            line_builder.bind_description(drawer->get_debug_desc());
            dp.render(line_builder, RenderPrimitiveType::LINES,
                      drawer->debug_mat->get_pipeline(), current_sort_key(1.0));
            RenderDrawDataBuilder triangle_builder =
                dp.generate_render_data(drawer->debug_mat);
            triangle_builder.bind_vertex_data(debug_triangle);
            triangle_builder.bind_description(drawer->get_debug_desc());
            dp.render(triangle_builder, RenderPrimitiveType::TRIANGLES,
                      drawer->debug_mat->get_pipeline(), current_sort_key(1.0));
            drawer->clear();
            drawer->unlock();
        }
    }
}
void DefaultRenderer::cleanup() {
    for (auto &[model, instances] : model_instances) {
        instances.clear();
    }

    entity_aabb.clear();
    this->seq = 0;
}

}  // namespace Seed