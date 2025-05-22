#include "model_renderer.h"
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

void ModelRenderer::init_color() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    instance_desc.add_attr(3, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(4, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(5, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(6, VertexAttributeType::FLOAT, 4, 1);

    debug_mat.create(DS::get_instance()->get_mesh_debug_shader());
    RenderRasterizerState rst = {.poly_mode = PolygonMode::LINE};
    debug_mat->set_rasterizer_state(rst);

    RenderResource lights_rc;

    Lights lights;
    lights.ambient = Vec3{0.8, 0.8, 0.8};
    lights.lights[0].set_position(Vec3{2, 0, 2});
    lights.lights[0].diffuse = Vec3{0.9, 0.5, 0.5};
    lights.lights[0].specular = Vec3{1, 1, 1};

    lights.lights[1].set_direction(Vec3{2, 3, -1});
    lights.lights[1].diffuse = Vec3{1, 1, 1};
    lights_rc.alloc_constant("Lights", sizeof(Lights), &lights);
    auto terrain_model = Mat4::translate_mat({0, 0, 0}).transpose();
    terrain_m.alloc_constant("TerrainMatrices", sizeof(Mat4), &terrain_model);
}

void ModelRenderer::init() {
    init_color();

    sky_vert.alloc_vertex(sizeof(Vec3), (sizeof(skyboxVertices) / sizeof(Vec3)),
                          skyboxVertices);
}

void ModelRenderer::preprocess() {
    std::vector<ModelEntity *> &entities =
        SeedEngine::get_instance()->get_world()->get_model_entities();
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

void ModelRenderer::process(Viewport &viewport) {
    RenderCommandDispatcher dp(layer);
    DEBUG_DISPATCH(dp);
    auto sky = SeedEngine::get_instance()->get_world()->get_sky();
    if (sky.is_valid()) {
        RenderDrawDataBuilder sky_builder =
            dp.generate_render_data(ref_cast<Material>(sky->get_material()));
        sky_builder.bind_vertex_data(sky_vert);
        sky_builder.bind_description(DS::get_instance()->get_sky_desc());
        dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
                  sky->get_material()->get_pipeline(), 1);
    }

    for (auto &[model, instances] : model_instances) {
        if (instances.empty()) {
            continue;
        }
        DEBUG_DISPATCH(dp);
        dp.update_buffer(model->instance_rc, 0, sizeof(Mat4) * instances.size(),
                         (void *)instances.data());
        for (Ref<Mesh> mesh : model->meshes) {
            RenderDrawDataBuilder mesh_builder = dp.generate_render_data(
                ref_cast<Material>(mesh->get_material()));
            mesh_builder.bind_vertex_data(mesh->vertex_data);
            mesh_builder.bind_description(DS::get_instance()->get_mesh_desc());
            mesh_builder.bind_vertex(model->instance_rc);
            mesh_builder.bind_description(&instance_desc);

            dp.render(mesh_builder, RenderPrimitiveType::TRIANGLES,
                      mesh->get_material()->get_pipeline(), 0.1);
        }
    }

    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    if (terrain.is_valid()) {
        RenderDrawDataBuilder builder = dp.generate_render_data(
            ref_cast<Material>(terrain->get_material()));
        builder.bind_vertex_data(*terrain->get_vertices(), 0);
        builder.bind_description(DS::get_instance()->get_terrain_desc());

        dp.render(builder, RenderPrimitiveType::PATCHES,
                  terrain->get_material()->get_pipeline(), 0.2);
    }
    {
        DebugDrawer *drawer = DebugDrawer::get_instance();
        RenderDrawDataBuilder line_builder = dp.generate_render_data(drawer->debug_mat);
        line_builder.bind_vertex_data(debug_line);
        line_builder.bind_description(drawer->get_debug_desc());
        dp.render(line_builder, RenderPrimitiveType::LINES, drawer->debug_mat->get_pipeline(), 0.3);
        RenderDrawDataBuilder triangle_builder = dp.generate_render_data(drawer->debug_mat);
        triangle_builder.bind_vertex_data(debug_triangle);
        triangle_builder.bind_description(drawer->get_debug_desc());
        dp.render(triangle_builder, RenderPrimitiveType::TRIANGLES, drawer->debug_mat->get_pipeline(), 0.3);
    }
}
void ModelRenderer::cleanup() {
    for (auto &[model, instances] : model_instances) {
        instances.clear();
    }

    entity_aabb.clear();
}

}  // namespace Seed