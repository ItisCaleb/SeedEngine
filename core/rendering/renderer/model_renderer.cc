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

    try {
        this->color_shader =
            loader->loadShader("assets/default.vert", "assets/default.frag");
    } catch (std::exception &e) {
        SPDLOG_ERROR("Error loading Shader: {}", e.what());
        exit(1);
    }
    this->vertices_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    this->vertices_desc.add_attr(1, VertexAttributeType::FLOAT, 3, 0);
    this->vertices_desc.add_attr(2, VertexAttributeType::FLOAT, 2, 0);
    this->instance_desc.add_attr(3, VertexAttributeType::FLOAT, 4, 1);
    this->instance_desc.add_attr(4, VertexAttributeType::FLOAT, 4, 1);
    this->instance_desc.add_attr(5, VertexAttributeType::FLOAT, 4, 1);
    this->instance_desc.add_attr(6, VertexAttributeType::FLOAT, 4, 1);

    u8 white_tex[4] = {255, 255, 255, 255};
    default_texture.create(1, 1, (const char *)white_tex);
    default_material.create();
    default_material->set_texture_map(Material::DIFFUSE, default_texture);
    default_material->set_texture_map(Material::SPECULAR, default_texture);
    default_material->set_texture_map(Material::NORMAl, default_texture);

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

    try {
        this->debugging_shader = loader->loadShader(
            "assets/debug.vert", "assets/debug.frag", "assets/debug.gs");
    } catch (std::exception &e) {
        SPDLOG_ERROR("Error loading Shader: {}", e.what());
        exit(1);
    }

    aabb_desc.add_attr(0, VertexAttributeType::FLOAT, 3, 0);
    aabb_desc.add_attr(1, VertexAttributeType::FLOAT, 3, 0);

    aabb_vertices_rc.alloc_vertex(sizeof(AABB), 0, NULL);
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
        if(cam && !cam->within_frustum(bounding_box)){
            continue;
        } 
        auto &instance = model_instances[*model];
        instance.push_back(e->get_transform().transpose());
        entity_aabb.push_back(bounding_box);
    }
}

void ModelRenderer::process(RenderCommandDispatcher &dp, u64 sort_key) {
    for (auto &[model, instances] : model_instances) {
        if(instances.empty()) {
            continue;
        }
        dp.begin(sort_key++);
        dp.use_vertex(&model->instance_rc, &this->instance_desc);

        dp.update(&model->instance_rc, 0, sizeof(Mat4) * instances.size(),
                  (void *)instances.data());
        for (Ref<Mesh> mesh : model->meshes) {
            dp.use_vertex(mesh->vertex_data.get_vertices(), &this->vertices_desc);
            dp.use(mesh->vertex_data.get_indices());
            Ref<Material> material = mesh->get_material();
            if (material.is_null()) {
                material = default_material;
            }
            Ref<Texture> diff_map = material->get_texture_map(Material::DIFFUSE);
            Ref<Texture> spec_map = material->get_texture_map(Material::SPECULAR);

            if (diff_map.is_valid()) {
                dp.use_texture(diff_map->get_render_resource(), Material::DIFFUSE);
            } else {
                dp.use_texture(default_texture->get_render_resource(), Material::DIFFUSE);
            }
            if (spec_map.is_valid()) {
                dp.use_texture(spec_map->get_render_resource(), Material::SPECULAR);
            } else {
                dp.use_texture(default_texture->get_render_resource(), Material::SPECULAR);
            }
            dp.render(&this->color_shader, RenderPrimitiveType::TRIANGLES,
                instances.size());
        }
        dp.end();
    }
    /* debugging */
    dp.begin(++sort_key);
    dp.use_vertex(&aabb_vertices_rc, &this->aabb_desc);
    dp.update(&aabb_vertices_rc, 0, sizeof(AABB) * entity_aabb.size(),
              (void *)entity_aabb.data());
    dp.render(&this->debugging_shader, RenderPrimitiveType::POINTS, 0, false);
    dp.end();
}
void ModelRenderer::cleanup() {
    for (auto &[model, instances] : model_instances) {
        instances.clear();
    }

    entity_aabb.clear();
}

}  // namespace Seed