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
    RenderCommandDispatcher dp;
    RenderStateDataBuilder builder;

    u_lights = RHI::alloc_constant(sizeof(STB140Lights),
                                   UpdateFrequence::PERFRAME, nullptr);
    u_csm = RHI::alloc_constant(sizeof(CSMShadow), UpdateFrequence::PERFRAME,
                                nullptr);
    builder.bind_constant(u_lights, 10);
    builder.bind_constant(u_csm, 11);
    dp.set_states(builder, 0);

    sky_vert.create(&DS::get_instance()->sky_desc,
                    (sizeof(skyboxVertices) / sizeof(Vec3)), skyboxVertices,
                    UpdateFrequence::STATIC);
    u32 shadow_map_resolution = shadow_map.get_resolution();

    Viewport shadow_map_vp(
        Vec2{(f32)shadow_map_resolution, (f32)shadow_map_resolution});
    shadow_map_rt.create(shadow_map_vp, true);
    shadow_map_rt->bind_depth(shadow_map.get_texture());
    shadow_map_dir_handle[0] = shadow_map.allocate_2048();
    for (u32 i = 1; i < CSM_SPLITS; i++) {
        shadow_map_dir_handle[i] = shadow_map.allocate_1024();
    }
    DebugDrawer *drawer = DebugDrawer::get_instance();

    debug_line.create(&drawer->debug_desc, UpdateFrequence::PERFRAME);
    debug_triangle.create(&drawer->debug_desc, UpdateFrequence::PERFRAME);
    debug_triangle_indices.create(std::vector<u32>{},
                                  UpdateFrequence::PERFRAME);
}

void DefaultRenderer::prepare_lights() {
    RenderCommandDispatcher dp;
    World *world = SeedEngine::get_instance()->get_world();
    DirectionalLight &dir_light = world->get_direction_light();
    Camera *cam = RenderEngine::get_instance()->get_cam();

    /* upload lights uniform*/
    STB140Lights *light_buf =
        (STB140Lights *)RHI::alloc_heap(sizeof(STB140Lights));
    dir_light.get_stb140(&light_buf->u_dir_light);
    light_buf->u_light_ambient = world->get_ambient_light();
    // for (u32 i = 0;
    //      i < (sizeof(light_buf->u_point_lights) / sizeof(STB140Light)); i++)
    //      {
    //     if (i < world->get_point_lights().size()) {
    //         world->get_point_lights()[i].get_stb140(
    //             &light_buf->u_point_lights[i]);

    //     } else {
    //         light_buf->u_point_lights[i].enable = 0.0f;
    //     }
    // }
    RHI::update_from_heap(u_lights, 0, sizeof(STB140Lights), light_buf);

    /* CSM frustum splits */
    CSMShadow *csm_data = (CSMShadow *)RHI::alloc_heap(sizeof(CSMShadow));
    std::vector<f32> resolutions;
    for (u32 i = 0; i < CSM_SPLITS; i++) {
        resolutions.push_back(
            shadow_map.query_viewport(shadow_map_dir_handle[i])
                .get_actual_dimension()
                .h);
    }
    dir_light.calculate_csm_lightspace(cam, resolutions, *csm_data);
    for (u32 i = 0; i < CSM_SPLITS; i++) {
        csm_data->shadow_uv[i] = shadow_map.query_uv(shadow_map_dir_handle[i]);
    }

    RHI::update_from_heap(u_csm, 0, sizeof(CSMShadow), csm_data);
}

void DefaultRenderer::prepare_meshes() {
    SSBOHandle visble_buffer = RenderEngine::get_instance()->visible_ssbo;

    World *world = SeedEngine::get_instance()->get_world();

    Camera *cam = RenderEngine::get_instance()->get_cam();
    DirectionalLight &dir_light = world->get_direction_light();

    MeshStorage *mesh_storage = MeshStorage::get_instance();
    std::set<InstanceData *> uploaded_instance;

    u32 last_visible_offset = 0;
    visible_instances.clear();
    for (auto &[mesh, instance] : mesh_storage->get_meshes()) {
        AABB bounding_box = mesh->get_bounding_box();

        /* check instance mesh size > 0 */
        if (!instance.is_null() && instance->get_size() == 0) {
            continue;
        }

        MeshInstance *color_mesh;
        if (mesh->get_material()->get_blend_state().blend_on) {
            color_mesh = &this->transparent_meshes.emplace_back(
                MeshInstance{.mesh = mesh});
        } else {
            color_mesh =
                &this->opaque_meshes.emplace_back(MeshInstance{.mesh = mesh});
        }
        ShadowMeshInstance &shadow_mesh =
            this->shadow_meshes.emplace_back(ShadowMeshInstance{.mesh = mesh});

        const Frustum &cam_frustum = cam->get_frustum();
        if (!instance.is_null()) {
            /* Use instancing */

            if (uploaded_instance.find(instance.ptr()) ==
                uploaded_instance.end()) {
                uploaded_instance.insert(instance.ptr());
                instance->upload();
            }
            color_mesh->visible_offset = visible_instances.size();
            instance->frustum_culling(cam_frustum, bounding_box,
                                      visible_instances, color_mesh->depth);
            color_mesh->visible_size =
                visible_instances.size() - color_mesh->visible_offset;
            u32 last_size = 0;
            for (u32 i = 0; i < CSM_SPLITS; i++) {
                shadow_mesh.visible_offset.push_back(visible_instances.size());
                instance->frustum_culling(dir_light.get_frustum(i),
                                          bounding_box, visible_instances,
                                          shadow_mesh.depth);
                shadow_mesh.visible_size.push_back(
                    visible_instances.size() -
                    shadow_mesh.visible_offset.back());
            }
        }
    }
    RHI::update(visble_buffer, 0, sizeof(u32) * visible_instances.size(),
                visible_instances.data());
}

void DefaultRenderer::preprocess() {
    prepare_lights();
    prepare_meshes();

    DebugDrawer *drawer = DebugDrawer::get_instance();

    World *world = SeedEngine::get_instance()->get_world();

    debug_line->update(drawer->line_vertices);
    debug_triangle->update(drawer->triangle_vertices);
    debug_triangle_indices->update(drawer->triangle_indices);
}

void DefaultRenderer::shadow_pass() {
    RenderCommandDispatcher dp;
    dp.begin_scope("Shadow Pass", current_sort_key());

    /* shadow pass */
    RenderStateDataBuilder shadow_map_state;
    u32 shadow_map_resolution = shadow_map.get_resolution();
    shadow_map_state.bind_render_target(shadow_map_rt->get_handle());
    dp.set_states(shadow_map_state, current_sort_key());
    shadow_map_state.reset();
    std::vector<Viewport> shadow_map_vps;
    for (u32 i = 0; i < CSM_SPLITS; i++) {
        Viewport *vp = &shadow_map_vps.emplace_back(
            shadow_map.query_viewport(shadow_map_dir_handle[i]));
        shadow_map_state.set_scissor(vp, true);
        shadow_map_state.clear(StateClearFlag::CLEAR_DEPTH);
        dp.set_states(shadow_map_state, current_sort_key());
        shadow_map_state.reset();
    }
    shadow_map_state.reset();
    shadow_map_state.set_scissor(shadow_map_rt->get_viewport());
    dp.set_states(shadow_map_state, current_sort_key());
    shadow_map_state.reset();

    Ref<Material> last_material;
    for (ShadowMeshInstance &mesh : shadow_meshes) {
        if (mesh.mesh->get_material()->get_shadow_pipeline() == NULL_HANDLE)
            continue;

        RenderDrawDataBuilder mesh_builder;
        if (last_material != mesh.mesh->get_material()) {
            mesh_builder = dp.generate_render_data(mesh.mesh->get_material());
            last_material = mesh.mesh->get_material();
        }
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);

        u32 last_size = 0;
        for (u32 i = 0; i < CSM_SPLITS; i++) {
            if (mesh.visible_size[i] == 0) continue;
            shadow_map_state.set_viewport(&shadow_map_vps[i], true);
            dp.set_states(shadow_map_state, current_sort_key());
            mesh_builder.push_constant(sizeof(u32), &mesh.visible_offset[i]);
            mesh_builder.push_constant(sizeof(u32), &i);
            mesh_builder.set_instance(mesh.visible_size[i], 0);
            dp.render(mesh_builder, mesh.mesh->get_type(),
                      mesh.mesh->get_material()->get_shadow_pipeline(),
                      current_sort_key());
            shadow_map_state.reset();
            mesh_builder.rollback();
            mesh_builder.rollback();
        }
    }

    dp.end_scope(next_sort_key());
}

void DefaultRenderer::color_pass(Viewport &viewport) {
    RenderCommandDispatcher dp;
    SSBOHandle visble_buffer = RenderEngine::get_instance()->visible_ssbo;
    dp.begin_scope("Color Pass", current_sort_key());
    RenderStateDataBuilder color_state;
    color_state.bind_render_target(RenderEngine::get_instance()
                                       ->get_render_target("default")
                                       ->get_handle());
    color_state.set_scissor(viewport.get_actual_dimension(false));
    color_state.set_viewport(&viewport);
    dp.set_states(color_state, current_sort_key());

    Ref<Material> last_material;
    for (MeshInstance &mesh : opaque_meshes) {
        if (mesh.visible_size == 0) continue;
        RenderDrawDataBuilder mesh_builder;
        mesh_builder.push_constant(sizeof(u32), &mesh.visible_offset);
        if (last_material != mesh.mesh->get_material()) {
            mesh_builder = dp.generate_render_data(mesh.mesh->get_material());
            mesh_builder.bind_texture(
                mesh.mesh->get_material()->get_texture_count(),
                shadow_map.get_texture()->get_handle());
            last_material = mesh.mesh->get_material();
        }
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.set_instance(mesh.visible_size);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
        dp.render(mesh_builder, mesh.mesh->get_type(),
                  mesh.mesh->get_material()->get_pipeline(),
                  current_sort_key());
    }

    for (MeshInstance &mesh : transparent_meshes) {
        if (mesh.visible_size == 0) continue;
        RenderDrawDataBuilder mesh_builder;
        mesh_builder.push_constant(sizeof(u32), &mesh.visible_offset);
        if (last_material != mesh.mesh->get_material()) {
            mesh_builder = dp.generate_render_data(mesh.mesh->get_material());
            mesh_builder.bind_texture(
                mesh.mesh->get_material()->get_texture_count(),
                shadow_map.get_texture()->get_handle());
            last_material = mesh.mesh->get_material();
        }
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
        for (u32 i = 0; i < mesh.visible_size; i++) {
            mesh_builder.set_instance(1, i);
            dp.render(mesh_builder, mesh.mesh->get_type(),
                      mesh.mesh->get_material()->get_pipeline(),
                      current_sort_key(mesh.depth[i]));
        }
    }

    auto sky = SeedEngine::get_instance()->get_world()->get_sky();
    if (sky.is_valid()) {
        RenderDrawDataBuilder sky_builder =
            dp.generate_render_data(ref_cast<Material>(sky->get_material()));
        sky_builder.bind_vertex_data(sky_vert);
        dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
                  sky->get_material()->get_pipeline(), current_sort_key(1.0));
    }
    dp.end_scope(next_sort_key());
}

void DefaultRenderer::debug_pass(Viewport &viewport) {
    RenderCommandDispatcher dp;

    DebugDrawer *drawer = DebugDrawer::get_instance();

    if (drawer->try_lock()) {
        RenderDrawDataBuilder line_builder =
            dp.generate_render_data(drawer->debug_mat);
        line_builder.bind_vertex_data(debug_line);
        dp.render(line_builder, RenderPrimitiveType::LINES,
                  drawer->debug_mat->get_pipeline(), current_sort_key());
        RenderDrawDataBuilder triangle_builder =
            dp.generate_render_data(drawer->debug_mat);
        triangle_builder.bind_vertex_data(debug_triangle);
        triangle_builder.bind_index_data(debug_triangle_indices);
        dp.render(triangle_builder, RenderPrimitiveType::TRIANGLES,
                  drawer->debug_mat->get_pipeline(), current_sort_key());
        drawer->clear();
        drawer->unlock();
    }
}

void DefaultRenderer::process(Viewport &viewport) {
    World *world = SeedEngine::get_instance()->get_world();

    RenderCommandDispatcher dp;
    dp.begin_scope("Default Rendering", current_sort_key());
    shadow_pass();
    color_pass(viewport);
    debug_pass(viewport);
    dp.end_scope(current_sort_key());
}
void DefaultRenderer::cleanup() {
    this->transparent_meshes.clear();
    this->opaque_meshes.clear();
    this->shadow_meshes.clear();

    entity_aabb.clear();
    this->seq = 0;
}

}  // namespace Seed