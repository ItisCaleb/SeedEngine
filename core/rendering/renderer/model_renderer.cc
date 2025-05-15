#include "model_renderer.h"
#include "core/rendering/light.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/resource_loader.h"
#include "core/engine.h"
#include <spdlog/spdlog.h>
#include <vector>

namespace Seed {

void ModelRenderer::init_color() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    Ref<Shader> color_shader =
        loader->load_shader("assets/default.vert", "assets/default.frag");
    color_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    color_desc.add_attr(1, VertexAttributeType::FLOAT, 3, 0);
    color_desc.add_attr(2, VertexAttributeType::FLOAT, 2, 0);
    instance_desc.add_attr(3, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(4, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(5, VertexAttributeType::FLOAT, 4, 1);
    instance_desc.add_attr(6, VertexAttributeType::FLOAT, 4, 1);

    RenderRasterizerState rst_state;
    RenderDepthStencilState depth_state = {.depth_on = true};
    RenderBlendState blend_state;

    this->color_pipeline.create(color_shader, &color_desc,
                                RenderPrimitiveType::TRIANGLES, rst_state,
                                depth_state, blend_state, &instance_desc);
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

    Ref<Shader> debug_shader = loader->load_shader(
        "assets/debug.vert", "assets/debug.frag", "assets/debug.gs");

    aabb_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    aabb_desc.add_attr(1, VertexAttributeType::FLOAT, 3, 0);

    RenderRasterizerState rst_state;
    RenderDepthStencilState depth_state = {.depth_on = true};
    RenderBlendState blend_state;

    debug_pipeline.create(debug_shader, &aabb_desc, RenderPrimitiveType::POINTS,
                          rst_state, depth_state, blend_state);
    RenderResource aabb_rc;
    aabb_rc.alloc_vertex(sizeof(AABB), 0, NULL);
    aabb_vertices.bind_vertices(sizeof(AABB), 0, aabb_rc);
}
void ModelRenderer::init() {
    init_color();
    init_debugging();
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

    dp.begin_draw();
    for (auto &[model, instances] : model_instances) {
        if (instances.empty()) {
            continue;
        }

        dp.update_buffer(model->instance_rc, 0,
                         sizeof(Mat4) * instances.size(),
                         (void *)instances.data());
        for (Ref<Mesh> mesh : model->meshes) {
            RenderDrawData data = dp.generate_render_data(
                mesh->vertex_data, mesh->get_material(), model->instance_rc,
                instances.size());
            dp.draw_set_viewport(data, 0, 0, window->get_width(),
                            window->get_height());

            dp.render(&data, color_pipeline, 0);
        }
    }
    dp.end_draw();
    /* debugging */
    RenderDrawData aabb_data =
        dp.generate_render_data(aabb_vertices, Ref<Material>());
    u64 sort_key = dp.gen_sort_key(0.1, aabb_data);
    dp.update_buffer(aabb_vertices.get_vertices(), 0,
                     sizeof(AABB) * entity_aabb.size(),
                     (void *)entity_aabb.data());
    aabb_vertices.bind_vertices(sizeof(AABB), entity_aabb.size(),
                                aabb_vertices.get_vertices());
    dp.begin_draw();

    dp.render(&aabb_data, debug_pipeline, 0.1);
    dp.end_draw();
}
void ModelRenderer::cleanup() {
    for (auto &[model, instances] : model_instances) {
        instances.clear();
    }

    entity_aabb.clear();
}

}  // namespace Seed