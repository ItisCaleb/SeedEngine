#include "default_renderer.h"
#include <set>
#include "core/rendering/instance_batch.h"
#include "core/rendering/light.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/engine.h"
#include <vector>
#include "core/debug/debug_drawer.h"
#include "core/rendering/mesh_storage.h"
#include "core/resource/default_storage.h"
#include "core/resource/material.h"
#include "core/system.h"
#include "core/world/world.h"

namespace Seed {

void DefaultRenderer::init(Window *window) {
    u32 res_w = window->get_width();
    u32 res_h = window->get_height();
    Ref<Texture> color_tex(TextureType::TEXTURE_2D, res_w, res_h,
                           PixelFormat::RGBA16F, MSAAType::SAMPLE_COUNT_4,
                           SamplerProperty{}, nullptr);
    Ref<Texture> depth_tex(
        TextureType::TEXTURE_2D, res_w, res_h, PixelFormat::D32S8,
        MSAAType::SAMPLE_COUNT_4,
        SamplerProperty{.min_filter = SamplerFilter::NEAREST,
                        .mag_filter = SamplerFilter::NEAREST},
        nullptr);

    fd.shadow_map_dir_handle[0] = fd.shadow_map.allocate_2048();
    for (u32 i = 1; i < CSM_SPLITS; i++) {
        fd.shadow_map_dir_handle[i] = fd.shadow_map.allocate_1024();
    }
    /* setup render passes */
    shadow_pass.setup(fd.shadow_map);
    color_pass.setup(color_tex, depth_tex);
    post_pass.setup(window);
    debug_pass.setup(color_pass);

    fd.post_mat.create(System::gDefaultStorage->post_shader);
    fd.post_mat->set_texture("image", color_tex);

    fd.debug_line.create(&System::gDebugDrawer->debug_desc,
                         UpdateFrequence::PERFRAME);
    fd.debug_triangle.create(&System::gDebugDrawer->debug_desc,
                             UpdateFrequence::PERFRAME);
    fd.debug_triangle_indices.create(std::vector<u32>{},
                                     UpdateFrequence::PERFRAME);
}

void DefaultRenderer::prepare_lights() {
    World *world = System::gEngine->get_world();
    DirectionalLight &dir_light = world->get_direction_light();
    Camera &cam = world->get_camera();
    FrameGlobal &g_frame = System::gRenderEngine->get_frame_global();
    /* fill main camera */
    RHI::UpdateBufferInfo cam_info =
        RHI::alloc_heap(sizeof(Camera::ShaderCamera) * 64);
    Camera::ShaderCamera *cams = (Camera::ShaderCamera *)cam_info.data;
    cam.fill_shader_camera(&cams[0]);
    /* upload lights uniform*/
    RHI::UpdateBufferInfo light_info = RHI::alloc_heap(sizeof(STB140Lights));
    STB140Lights *light_buf = (STB140Lights *)light_info.data;
    dir_light.get_stb140(&light_buf->u_dir_light);
    light_buf->u_light_ambient = world->get_ambient_light();
    std::vector<PointLight> &point_lights = world->get_point_lights();
    u32 point_light_size =
        (sizeof(light_buf->u_point_lights) / sizeof(STB140Light));
    for (u32 i = 0; i < point_light_size; i++) {
        if (i < point_lights.size()) {
            point_lights[i].get_stb140(&light_buf->u_point_lights[i]);
        } else {
            light_buf->u_point_lights[i].enable = 0.0f;
        }
    }
    RHI::update_from_heap(g_frame.lights, 0, light_info);

    /* CSM frustum splits */
    RHI::UpdateBufferInfo csm_info = RHI::alloc_heap(sizeof(CSMShadow));
    CSMShadow *csm_data = (CSMShadow *)csm_info.data;
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
    /* fill cams[5 ~ 40] to directional light*/
    for (u32 i = 0; i < 6; i++) {
        if (i >= point_lights.size()) {
            break;
        }
        point_lights[i].calculate_lightspace(&cams[5 + i * 6]);
    }

    RHI::update_from_heap(g_frame.csm, 0, csm_info);
    RHI::update_from_heap(g_frame.camera, 0, cam_info);
}

void DefaultRenderer::prepare_meshes() {
    World *world = System::gEngine->get_world();

    Camera &cam = world->get_camera();
    DirectionalLight &dir_light = world->get_direction_light();
    FrameGlobal &g_frame = System::gRenderEngine->get_frame_global();

    MeshStorage *mesh_storage = System::gRenderEngine->get_mesh_storage();
    std::vector<u32> visible_instances;

    u32 last_visible_offset = 0;
    for (auto &[_, mesh_instance] : mesh_storage->get_meshes()) {
        Ref<Mesh> mesh = mesh_instance.mesh;
        Ref<InstanceBatch> instance = mesh_instance.instance;
        AABB bounding_box = mesh->get_bounding_box();

        /* check instance mesh size > 0 */
        if (instance.is_null() || instance->size() == 0) {
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

        const Frustum &cam_frustum = cam.get_frustum();
        InstanceBatchPool *pool =
            System::gRenderEngine->get_instance_pool(instance);
        instance->upload();
        color_mesh->visible_offset = visible_instances.size();
        instance->frustum_culling(cam_frustum, bounding_box, visible_instances,
                                  color_mesh->depth);
        color_mesh->visible_size =
            visible_instances.size() - color_mesh->visible_offset;

        /* check mesh cast shadow */
        if (mesh->get_material()->do_cast_shadow()) {
            ShadowMeshInstance &shadow_mesh =
                this->fd.shadow_meshes.emplace_back(
                    ShadowMeshInstance{.mesh = mesh});
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
    RHI::update(g_frame.visible, 0, sizeof(u32) * visible_instances.size(),
                visible_instances.data());
}

void DefaultRenderer::preprocess() {
    prepare_lights();
    prepare_meshes();

    fd.debug_line->update(System::gDebugDrawer->line_vertices);
    fd.debug_triangle->update(System::gDebugDrawer->triangle_vertices);
    fd.debug_triangle_indices->update(System::gDebugDrawer->triangle_indices);
}

void DefaultRenderer::_process(RenderCommandDispatcher &dp) {
    dp.begin_scope("Default Rendering");
    dp.set_seq(0);
    shadow_pass.draw(dp, fd);
    dp.set_seq(1);
    color_pass.draw(dp, fd);
    dp.set_seq(2);
    debug_pass.draw(dp, fd);
    dp.set_seq(3);
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
        Ref<Material> material = mesh.mesh->get_material();
        if (last_material != material) {
            material->upload_parameter(dp);
            mesh_builder = dp.generate_render_data(material);
            last_material = material;
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
            mesh_builder.set_depth_clamp(true);
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
                material->upload_parameter(dp);
                mesh_builder = dp.generate_render_data(material);
                if (material->do_receive_shadow()) {
                    mesh_builder.bind_texture(
                        material->get_texture_count(),
                        fd.shadow_map.get_texture()->get_handle());
                }
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
            material->upload_parameter(dp);
            mesh_builder = dp.generate_render_data(material);
            i16 shadow_map_unit = material->get_shadow_map_unit();
            if (material->do_receive_shadow() && shadow_map_unit != -1) {
                mesh_builder.bind_texture(
                    shadow_map_unit, fd.shadow_map.get_texture()->get_handle());
            }

            last_material = material;
        }
        mesh_builder.push_constant(mesh.visible_offset);
        mesh_builder.push_constant(0);
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.set_instance(mesh.visible_size);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
        mesh_builder.set_depth_test(CompareOP::EQUAL);

        dp.render(mesh_builder, mesh.mesh->get_type(), material->get_pipeline(),
                  0);
    }

    for (MeshInstance &mesh : fd.transparent_meshes) {
        if (mesh.visible_size == 0) continue;
        RenderDrawDataBuilder mesh_builder;
        if (last_material != mesh.mesh->get_material()) {
            mesh_builder = dp.generate_render_data(mesh.mesh->get_material());
            // mesh_builder.bind_texture(
            //     mesh.mesh->get_material()->get_texture_count(),
            //     fd.shadow_map.get_texture()->get_handle());
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

    auto sky = System::gEngine->get_world()->get_sky();
    if (sky.is_valid()) {
        RenderDrawDataBuilder sky_builder =
            dp.generate_render_data(ref_cast<Material>(sky->get_material()));
        sky_builder.push_constant(0);
        sky_builder.push_constant(0);
        sky_builder.bind_vertex_data(System::gDefaultStorage->sky_vertices);
        sky_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);
        dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
                  sky->get_material()->get_pipeline(), 1.0);
    }
}

void DefaultRenderer::DebugPass::execute(RenderCommandDispatcher &dp,
                                         Viewport &viewport, FrameData &fd) {
    DebugDrawer *drawer = System::gDebugDrawer;

    if (drawer->try_lock()) {
        if (fd.debug_line->get_count() != 0) {
            RenderDrawDataBuilder line_builder =
                dp.generate_render_data(drawer->debug_mat);
            line_builder.bind_vertex_data(fd.debug_line);
            dp.render(line_builder, RenderPrimitiveType::LINES,
                      drawer->debug_mat->get_pipeline(), 0);
        }
        if (fd.debug_triangle->get_count() != 0) {
            RenderDrawDataBuilder triangle_builder =
                dp.generate_render_data(drawer->debug_mat);
            triangle_builder.bind_vertex_data(fd.debug_triangle);
            triangle_builder.bind_index_data(fd.debug_triangle_indices);
            dp.render(triangle_builder, RenderPrimitiveType::TRIANGLES,
                      drawer->debug_mat->get_pipeline(), 0);
        }

        drawer->clear();
        drawer->unlock();
    }
}

void DefaultRenderer::PostPass::execute(RenderCommandDispatcher &dp,
                                        Viewport &viewport, FrameData &fd) {
    auto builder = dp.generate_render_data(fd.post_mat);
    builder.bind_vertex_data(System::gDefaultStorage->quad_vertices);
    dp.render(builder, RenderPrimitiveType::TRIANGLES,
              fd.post_mat->get_pipeline(), 0);
}

}  // namespace Seed
