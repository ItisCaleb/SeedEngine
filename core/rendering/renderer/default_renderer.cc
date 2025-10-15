#include "default_renderer.h"
#include "core/rendering/light.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/resource_loader.h"
#include "core/rendering/api/render_engine.h"
#include "core/engine.h"
#include <spdlog/spdlog.h>
#include <vector>
#include "core/debug/debug_drawer.h"
#include "core/rendering/mesh_storage.h"

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

void DefaultRenderer::init() {
    ResourceLoader *loader = ResourceLoader::get_instance();

    instance_desc.add_type_attr<u32>(3, 1);
    instance_idx_rc.alloc_vertex(sizeof(u32), 0, nullptr);

    debug_mat.create(DS::get_instance()->mesh_debug_shader);
    RenderRasterizerState rst = {.poly_mode = PolygonMode::LINE};
    debug_mat->set_rasterizer_state(rst);

    u_lights.alloc_constant("Lights", sizeof(STB140Lights), nullptr);
    u_lightspaces.alloc_constant("LightSpaceMatrices", sizeof(Mat4) * 9,
                                 nullptr);
    shadow_map_default_pipeline.alloc_pipeline(
        DS::get_instance()->shadow_default_shader->get_render_resource(),
        RenderRasterizerState{.cull_mode = Cullmode::FRONT},
        RenderDepthStencilState{.depth_on = true}, {});
    shadow_map_terrain_pipeline.alloc_pipeline(
        DS::get_instance()->shadow_terrain_shader->get_render_resource(),
        RenderRasterizerState{.cull_mode = Cullmode::FRONT,
                              .patch_control_points = 4},
        RenderDepthStencilState{.depth_on = true}, {});
    auto terrain_model = Mat4::translate_mat({0, 0, 0}).transpose();
    terrain_m.alloc_constant("TerrainMatrices", sizeof(Mat4), &terrain_model);

    sky_vert.alloc_vertex(sizeof(Vec3), (sizeof(skyboxVertices) / sizeof(Vec3)),
                          skyboxVertices);

    shadow_map_rt.create(true);
    shadow_map_rt->bind_depth(shadow_map.get_texture());
    shadow_map_dir_handle = shadow_map.allocate_2048();
}

void DefaultRenderer::preprocess() {
    RenderCommandDispatcher dp;

    World *world = SeedEngine::get_instance()->get_world();

    Camera *cam = RenderEngine::get_instance()->get_cam();
    MeshStorage *mesh_storage = MeshStorage::get_instance();
    std::set<InstanceData *> uploaded_instance;

    for (auto &[mesh, instance] : mesh_storage->get_meshes()) {
        AABB bounding_box = mesh->get_bounding_box();
        if (uploaded_instance.find(instance.ptr()) == uploaded_instance.end()) {
            uploaded_instance.insert(instance.ptr());
            instance->upload();
        }
        MeshInstance *mesh_inst;
        if (mesh->get_material()->get_blend_state().blend_on) {
            this->transparent_meshes.push_back({.mesh = mesh});
            mesh_inst =
                &this->transparent_meshes[this->transparent_meshes.size() - 1];
        } else {
            this->opaque_meshes.push_back({.mesh = mesh});
            mesh_inst = &this->opaque_meshes[this->opaque_meshes.size() - 1];
        }
        u32 i = instance->get_start_idx() - 1;
        for (Ref<Transform> transform : instance->get_transforms()) {
            AABB aabb = transform->translate_AABB(bounding_box);
            i++;
            /* frustum culling */
            if (cam && cam->within_frustum(aabb)) {
                /* push instance indices */
                mesh_inst->instance_id.push_back(i);
                mesh_inst->depth.push_back(
                    cam->calculate_depth(transform->get_position()));
            }
        }
    }

    /* upload lights uniform*/
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
    Mat4 *light_mats = (Mat4 *)dp.map_buffer(u_lightspaces, 0, sizeof(Mat4) * 9,
                                             current_sort_key());
    light_mats[0] =
        world->get_direction_light().get_light_space_mat().transpose();
    for (u32 i = 0; i < 8 && i < world->get_point_lights().size(); i++) {
        light_mats[i + 1] =
            world->get_point_lights()[i].get_light_space_mat().transpose();
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

void DefaultRenderer::shadow_pass() {
    RenderCommandDispatcher dp;
    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    dp.begin_scope("Shadow Pass", current_sort_key());

    /* shadow pass */
    RenderStateDataBuilder shadow_map_state;

    shadow_map_state.bind_render_target(shadow_map_rt->get_resource());
    shadow_map_state.set_scissor(0, 0, shadow_map.get_resolution(),
                                 shadow_map.get_resolution());

    shadow_map_state.clear(StateClearFlag::CLEAR_DEPTH);
    shadow_map_state.set_viewport(
        shadow_map.query_viewport(shadow_map_dir_handle),
        shadow_map.get_resolution());
    shadow_map_state.bind_bufferbase(
        InstanceDataPool::get_instance()->get_render_buffer(), 0);
    dp.set_states(shadow_map_state, current_sort_key());

    for (MeshInstance &mesh : opaque_meshes) {
        if (mesh.instance_id.empty()) continue;
        dp.update_buffer(instance_idx_rc, 0,
                         sizeof(u32) * mesh.instance_id.size(),
                         (void *)mesh.instance_id.data());
        RenderDrawDataBuilder mesh_builder;
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.bind_description(&DS::get_instance()->mesh_desc);
        mesh_builder.bind_vertex(instance_idx_rc);
        mesh_builder.bind_description(&instance_desc);
        mesh_builder.set_instance(mesh.instance_id.size());

        dp.render(mesh_builder, RenderPrimitiveType::TRIANGLES,
                  shadow_map_default_pipeline, current_sort_key());
    }

    // if (terrain.is_valid() && terrain->is_loaded()) {
    //     for (TerrainChunk &chunk : terrain->get_chunks()) {
    //         RenderDrawDataBuilder builder = dp.generate_render_data(
    //             ref_cast<Material>(terrain->get_material()));
    //         builder.bind_vertex_data(chunk.vertex_data, 0);
    //         builder.bind_description(&DS::get_instance()->terrain_desc);
    //         dp.render(builder, RenderPrimitiveType::PATCHES,
    //                   shadow_map_terrain_pipeline, current_sort_key(1));
    //     }
    // }

    dp.end_scope(next_sort_key());
}

void DefaultRenderer::color_pass(Viewport &viewport) {
    RenderCommandDispatcher dp;
    Ref<Terrain> terrain =
        SeedEngine::get_instance()->get_world()->get_terrain();
    dp.begin_scope("Color Pass", current_sort_key());
    RenderStateDataBuilder color_state;
    color_state.bind_render_target(RenderEngine::get_instance()
                                       ->get_render_target("default")
                                       ->get_resource());
    color_state.set_scissor(viewport.get_actual_dimension());
    color_state.set_viewport(viewport.get_actual_dimension());
    color_state.bind_bufferbase(
        InstanceDataPool::get_instance()->get_render_buffer(), 0);
    dp.set_states(color_state, current_sort_key());

    for (MeshInstance &mesh : opaque_meshes) {
        if (mesh.instance_id.empty()) continue;
        dp.update_buffer(instance_idx_rc, 0,
                         sizeof(u32) * mesh.instance_id.size(),
                         (void *)mesh.instance_id.data());
        RenderDrawDataBuilder mesh_builder = dp.generate_render_data(
            ref_cast<Material>(mesh.mesh->get_material()));
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.bind_description(&DS::get_instance()->mesh_desc);
        mesh_builder.bind_vertex(instance_idx_rc);
        mesh_builder.bind_description(&instance_desc);
        mesh_builder.set_instance(mesh.instance_id.size());
        dp.render(mesh_builder, RenderPrimitiveType::TRIANGLES,
                  mesh.mesh->get_material()->get_pipeline(),
                  current_sort_key());
    }

    // if (terrain.is_valid() && terrain->is_loaded()) {
    //     for (TerrainChunk &chunk : terrain->get_chunks()) {
    //         // /* frustum culling */
    //         // if (cam && cam->within_frustum(aabb)) {
    //         //     /* push instance indices */
    //         //     mesh_inst->instance_id.push_back(i);
    //         //     mesh_inst->depth.push_back(
    //         //         cam->calculate_depth(transform->get_position()));
    //         // }
    //         RenderDrawDataBuilder builder = dp.generate_render_data(
    //             ref_cast<Material>(terrain->get_material()));
    //         builder.bind_texture(1, shadow_map.get_texture()->get_resource());
    //         builder.bind_vertex_data(chunk.vertex_data, 0);
    //         builder.bind_description(&DS::get_instance()->terrain_desc);
    //         dp.render(builder, RenderPrimitiveType::PATCHES,
    //                   terrain->get_material()->get_pipeline(),
    //                   current_sort_key(1));
    //     }
    // }

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
}

void DefaultRenderer::process(Viewport &viewport) {
    World *world = SeedEngine::get_instance()->get_world();

    RenderCommandDispatcher dp;
    dp.begin_scope("Default Rendering", current_sort_key());
    shadow_pass();
    color_pass(viewport);
    dp.end_scope(current_sort_key());

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
    this->transparent_meshes.clear();
    this->opaque_meshes.clear();

    entity_aabb.clear();
    this->seq = 0;
}

}  // namespace Seed