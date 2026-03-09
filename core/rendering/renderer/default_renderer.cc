#include "default_renderer.h"
#include "core/rendering/light.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/rendering/rhi/render_engine.h"
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

void DefaultRenderer::init(Window *window) {
    RenderEngine *engine = RenderEngine::get_instance();
    u32 res_w = window->get_width();
    u32 res_h = window->get_height();
    Ref<Texture> color_tex(TextureType::TEXTURE_2D, res_w, res_h,
                           PixelFormat::RGBA16F, nullptr);
    Ref<Texture> depth_tex(
        TextureType::TEXTURE_2D, res_w, res_h, PixelFormat::D32S8, nullptr,
        SamplerProperty{.min_filter = SamplerFilter::NEAREST,
                        .mag_filter = SamplerFilter::NEAREST});

    fd.shadow_map_dir_handle[0] = fd.shadow_map.allocate_2048();
    for (u32 i = 1; i < CSM_SPLITS; i++) {
        fd.shadow_map_dir_handle[i] = fd.shadow_map.allocate_1024();
    }
    /* setup render passes */
    shadow_pass.setup(fd.shadow_map);
    color_pass.setup(color_tex, depth_tex);
    post_pass.setup(window);

    fd.post_mat.create(DS::get_instance()->post_shader);
    fd.post_mat->set_texture("image", color_tex);
    visible_ssbo = RHI::alloc_storage_buffer(sizeof(int) * 65536,
                                             UpdateFrequence::PERFRAME);

    transform_ssbo =
        engine->get_instance_pool(TRANSFORM_POOL_NAME)->get_render_buffer();
    terrain_ssbo =
        engine->get_instance_pool(TERRAIN_POOL_NAME)->get_render_buffer();
    bone_ssbo =
        engine->get_instance_pool(SKELETON_POOL_NAME)->get_render_buffer();
    camera = RHI::alloc_constant(sizeof(Camera::ShaderCamera) * 8,
                                 UpdateFrequence::PERFRAME);
    u_lights =
        RHI::alloc_constant(sizeof(STB140Lights), UpdateFrequence::PERFRAME);
    u_csm = RHI::alloc_constant(sizeof(CSMShadow), UpdateFrequence::PERFRAME);

    fd.sky_vert.create(&DS::get_instance()->sky_desc,
                       (sizeof(skyboxVertices) / sizeof(Vec3)), skyboxVertices,
                       UpdateFrequence::STATIC);

    DebugDrawer *drawer = DebugDrawer::get_instance();

    fd.debug_line.create(&drawer->debug_desc, UpdateFrequence::PERFRAME);
    fd.debug_triangle.create(&drawer->debug_desc, UpdateFrequence::PERFRAME);
    fd.debug_triangle_indices.create(std::vector<u32>{},
                                     UpdateFrequence::PERFRAME);
}

void DefaultRenderer::prepare_lights() {
    World *world = SeedEngine::get_instance()->get_world();
    DirectionalLight &dir_light = world->get_direction_light();
    Camera &cam = world->get_camera();

    /* fill main camera */
    Camera::ShaderCamera *cams = (Camera::ShaderCamera *)RHI::alloc_heap(
        sizeof(Camera::ShaderCamera) * 8);
    cam.fill_shader_camera(&cams[0]);
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
            fd.shadow_map.query_viewport(fd.shadow_map_dir_handle[i])
                .get_actual_dimension()
                .h);
    }
    /* fill cams[1 ~ 4] to CSM */
    dir_light.calculate_csm_lightspace(cam, resolutions, *csm_data, &cams[1]);
    for (u32 i = 0; i < CSM_SPLITS; i++) {
        csm_data->shadow_uv[i] =
            fd.shadow_map.query_uv(fd.shadow_map_dir_handle[i]);
    }

    RHI::update_from_heap(u_csm, 0, sizeof(CSMShadow), csm_data);
    RHI::update_from_heap(camera, 0, sizeof(Camera::ShaderCamera) * 8, cams);
}

void DefaultRenderer::prepare_meshes() {
    World *world = SeedEngine::get_instance()->get_world();

    Camera &cam = world->get_camera();
    DirectionalLight &dir_light = world->get_direction_light();

    MeshStorage *mesh_storage = MeshStorage::get_instance();
    std::set<InstanceData *> uploaded_instance;
    std::vector<u32> visible_instances;

    u32 last_visible_offset = 0;
    for (auto &[mesh, instance] : mesh_storage->get_meshes()) {
        AABB bounding_box = mesh->get_bounding_box();

        /* check instance mesh size > 0 */
        if (!instance.is_null() && instance->size() == 0) {
            continue;
        }

        MeshInstance *color_mesh;
        if (mesh->get_material()->get_blend_state().blend_on) {
            color_mesh = &this->fd.transparent_meshes.emplace_back(
                MeshInstance{.mesh = mesh});
        } else {
            color_mesh = &this->fd.opaque_meshes.emplace_back(
                MeshInstance{.mesh = mesh});
        }
        ShadowMeshInstance &shadow_mesh = this->fd.shadow_meshes.emplace_back(
            ShadowMeshInstance{.mesh = mesh});

        const Frustum &cam_frustum = cam.get_frustum();
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
    RHI::update(visible_ssbo, 0, sizeof(u32) * visible_instances.size(),
                visible_instances.data());
}

void DefaultRenderer::preprocess() {
    World *world = SeedEngine::get_instance()->get_world();

    prepare_lights();
    prepare_meshes();

    DebugDrawer *drawer = DebugDrawer::get_instance();

    fd.debug_line->update(drawer->line_vertices);
    fd.debug_triangle->update(drawer->triangle_vertices);
    fd.debug_triangle_indices->update(drawer->triangle_indices);

    RenderCommandDispatcher dp;
    RenderStateDataBuilder builder;
    builder.bind_storage_buffer(visible_ssbo, 0);
    builder.bind_storage_buffer(transform_ssbo, 1);
    builder.bind_storage_buffer(terrain_ssbo, 2);
    builder.bind_storage_buffer(bone_ssbo, 3);
    builder.bind_constant(camera, 8);
    builder.bind_constant(u_lights, 10);
    builder.bind_constant(u_csm, 11);

    dp.set_states(builder);
}

void DefaultRenderer::_process(RenderCommandDispatcher &dp) {
    dp.begin_scope("Default Rendering");
    dp.set_seq(0);
    shadow_pass.draw(dp, fd);
    dp.set_seq(1);
    color_pass.draw(dp, fd);
    dp.set_seq(2);
    post_pass.draw(dp, fd);
    dp.end_scope();
}
void DefaultRenderer::cleanup() {
    this->fd.transparent_meshes.clear();
    this->fd.opaque_meshes.clear();
    this->fd.shadow_meshes.clear();

    entity_aabb.clear();
}

void DefaultRenderer::ShadowPass::execute(RenderCommandDispatcher &dp,
                                          Viewport &viewport, FrameData &fd) {
    /* shadow pass */
    RenderStateDataBuilder shadow_map_state;
    u32 shadow_map_resolution = fd.shadow_map.get_resolution();
    std::vector<Viewport> shadow_map_vps;
    for (u32 i = 0; i < CSM_SPLITS; i++) {
        Viewport *vp = &shadow_map_vps.emplace_back(
            fd.shadow_map.query_viewport(fd.shadow_map_dir_handle[i]));
        shadow_map_state.set_scissor(vp, true);
        shadow_map_state.clear(StateClearFlagBits::CLEAR_DEPTH);
        dp.set_states(shadow_map_state);
        shadow_map_state.reset();
    }
    shadow_map_state.reset();
    shadow_map_state.set_scissor(0, 0, shadow_map_resolution,
                                 shadow_map_resolution);
    dp.set_states(shadow_map_state);
    shadow_map_state.reset();

    Ref<Material> last_material;
    for (ShadowMeshInstance &mesh : fd.shadow_meshes) {
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
            dp.set_states(shadow_map_state);
            mesh_builder.push_constant(mesh.visible_offset[i]);
            mesh_builder.push_constant(i + 1);
            mesh_builder.set_instance(mesh.visible_size[i], 0);
            mesh_builder.set_depth_write(true);
            /* alpha test need fragment shader */
            if (mesh.mesh->get_material()->get_depth_state().depth_mode !=
                DepthMode::ALPHA_TEST) {
                mesh_builder.set_draw_depth_only(true);
            }
            mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);
            dp.render(mesh_builder, mesh.mesh->get_type(),
                      mesh.mesh->get_material()->get_pipeline(), 0);
            shadow_map_state.reset();
            mesh_builder.rollback();
            mesh_builder.rollback();
        }
    }
}

void DefaultRenderer::ColorPass::execute(RenderCommandDispatcher &dp,
                                         Viewport &viewport, FrameData &fd) {
    /* depth prepass */
    {
        Ref<Material> last_material;
        for (MeshInstance &mesh : fd.opaque_meshes) {
            if (mesh.visible_size == 0) continue;
            RenderDrawDataBuilder mesh_builder;
            Ref<Material> material = mesh.mesh->get_material();
            DepthMode depth_mode = material->get_depth_state().depth_mode;
            if (last_material != material) {
                mesh_builder = dp.generate_render_data(material);
                mesh_builder.bind_texture(
                    material->get_texture_count(),
                    fd.shadow_map.get_texture()->get_handle());
                last_material = material;
            }
            mesh_builder.push_constant(mesh.visible_offset);
            mesh_builder.push_constant(0);
            mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
            mesh_builder.set_instance(mesh.visible_size);
            mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
            mesh_builder.set_depth_write(true);
            /* alpha test need fragment shader */
            if (mesh.mesh->get_material()->get_depth_state().depth_mode !=
                DepthMode::ALPHA_TEST) {
                mesh_builder.set_draw_depth_only(true);
            }
            mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);

            dp.render(mesh_builder, mesh.mesh->get_type(),
                      material->get_pipeline(), 0);
        }
    }

    Ref<Material> last_material;
    for (MeshInstance &mesh : fd.opaque_meshes) {
        if (mesh.visible_size == 0) continue;
        RenderDrawDataBuilder mesh_builder;
        Ref<Material> material = mesh.mesh->get_material();
        DepthMode depth_mode = material->get_depth_state().depth_mode;
        if (last_material != material) {
            mesh_builder = dp.generate_render_data(material);
            mesh_builder.bind_texture(
                material->get_texture_count(),
                fd.shadow_map.get_texture()->get_handle());
            last_material = material;
        }
        mesh_builder.push_constant(mesh.visible_offset);
        mesh_builder.push_constant(0);
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.set_instance(mesh.visible_size);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
        mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);

        dp.render(mesh_builder, mesh.mesh->get_type(), material->get_pipeline(),
                  0);
    }

    for (MeshInstance &mesh : fd.transparent_meshes) {
        if (mesh.visible_size == 0) continue;
        RenderDrawDataBuilder mesh_builder;
        if (last_material != mesh.mesh->get_material()) {
            mesh_builder = dp.generate_render_data(mesh.mesh->get_material());
            mesh_builder.bind_texture(
                mesh.mesh->get_material()->get_texture_count(),
                fd.shadow_map.get_texture()->get_handle());
            last_material = mesh.mesh->get_material();
        }
        mesh_builder.push_constant(mesh.visible_offset);
        mesh_builder.push_constant(0);

        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
        mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);

        for (u32 i = 0; i < mesh.visible_size; i++) {
            mesh_builder.set_instance(1, i);

            dp.render(mesh_builder, mesh.mesh->get_type(),
                      mesh.mesh->get_material()->get_pipeline(), mesh.depth[i]);
        }
    }

    auto sky = SeedEngine::get_instance()->get_world()->get_sky();
    if (sky.is_valid()) {
        RenderDrawDataBuilder sky_builder =
            dp.generate_render_data(ref_cast<Material>(sky->get_material()));
        sky_builder.push_constant(0);
        sky_builder.push_constant(0);
        sky_builder.bind_vertex_data(fd.sky_vert);
        sky_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);
        dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
                  sky->get_material()->get_pipeline(), 1.0);
    }
}

void DefaultRenderer::DebugPass::execute(RenderCommandDispatcher &dp,
                                         Viewport &viewport, FrameData &fd) {
    DebugDrawer *drawer = DebugDrawer::get_instance();

    if (drawer->try_lock()) {
        RenderDrawDataBuilder line_builder =
            dp.generate_render_data(drawer->debug_mat);
        line_builder.bind_vertex_data(fd.debug_line);
        dp.render(line_builder, RenderPrimitiveType::LINES,
                  drawer->debug_mat->get_pipeline(), 0);
        RenderDrawDataBuilder triangle_builder =
            dp.generate_render_data(drawer->debug_mat);
        triangle_builder.bind_vertex_data(fd.debug_triangle);
        triangle_builder.bind_index_data(fd.debug_triangle_indices);
        dp.render(triangle_builder, RenderPrimitiveType::TRIANGLES,
                  drawer->debug_mat->get_pipeline(), 0);
        drawer->clear();
        drawer->unlock();
    }
}

void DefaultRenderer::PostPass::execute(RenderCommandDispatcher &dp,
                                        Viewport &viewport, FrameData &fd) {
    auto builder = dp.generate_render_data(fd.post_mat);
    builder.bind_vertex_data(DS::get_instance()->quad_vertices);
    dp.render(builder, RenderPrimitiveType::TRIANGLES,
              fd.post_mat->get_pipeline(), 0);
}

}  // namespace Seed