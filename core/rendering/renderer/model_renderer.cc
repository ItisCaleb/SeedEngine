#include "model_renderer.h"
#include "core/rendering/light.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/resource_loader.h"
#include "core/engine.h"
#include <spdlog/spdlog.h>
#include <vector>

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
    RenderResource lights_rc;

    Lights lights;
    lights.ambient = Vec3{0.8, 0.8, 0.8};
    lights.lights[0].set_position(Vec3{2, 0, 2});
    lights.lights[0].diffuse = Vec3{0.9, 0.5, 0.5};
    lights.lights[0].specular = Vec3{1, 1, 1};

    lights.lights[1].set_direction(Vec3{2, 3, -1});
    lights.lights[1].diffuse = Vec3{1, 1, 1};
    lights_rc.alloc_constant("Lights", sizeof(Lights), &lights);
}
void ModelRenderer::init_debugging() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    // aabb_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    // aabb_desc.add_attr(1, VertexAttributeType::FLOAT, 3, 0);

    // RenderRasterizerState rst_state;
    // RenderDepthStencilState depth_state = {.depth_on = true};
    // RenderBlendState blend_state;

    // debug_pipeline.create(debug_shader, &aabb_desc,
    // RenderPrimitiveType::POINTS,
    //                       rst_state, depth_state, blend_state);
    // RenderResource aabb_rc;
    // aabb_rc.alloc_vertex(sizeof(AABB), 0, NULL);
    // aabb_vertices.bind_vertices(sizeof(AABB), 0, aabb_rc);
}
void ModelRenderer::init() {
    init_color();
    init_debugging();

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
}

void ModelRenderer::process() {
    Window *window = SeedEngine::get_instance()->get_window();
    RenderCommandDispatcher dp(layer);
    DEBUG_DISPATCH(dp);
    dp.set_viewport(0, 0, window->get_width(), window->get_height());
    auto sky = SeedEngine::get_instance()->get_world()->get_sky();
    RenderDrawDataBuilder sky_builder =
        dp.generate_render_data(ref_cast<Material>(sky->get_material()));
    sky_builder.bind_vertex_data(sky_vert);
    sky_builder.bind_description(DS::get_instance()->get_sky_desc());
    dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
              sky->get_material()->get_pipeline(), 0);

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
    /* debugging */
    // RenderDrawData aabb_data =
    //     dp.generate_render_data(aabb_vertices, Ref<Material>());
    // u64 sort_key = dp.gen_sort_key(0.1, aabb_data);
    // dp.update_buffer(aabb_vertices.get_vertices(), 0,
    //                  sizeof(AABB) * entity_aabb.size(),
    //                  (void *)entity_aabb.data());
    // aabb_vertices.bind_vertices(sizeof(AABB), entity_aabb.size(),
    //                             aabb_vertices.get_vertices());

    // dp.render(&aabb_data, debug_pipeline, 0.2);
}
void ModelRenderer::cleanup() {
    for (auto &[model, instances] : model_instances) {
        instances.clear();
    }

    entity_aabb.clear();
}

}  // namespace Seed