#include "world_renderer.h"
#include "editor/world/editor_world.h"
#include <cstring>
#include "editor/editor.h"
#include "core/engine.h"
#include "core/rendering/light.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/system.h"
#include "core/world/world.h"

namespace Seed {
WorldRenderer::WorldRenderer(u32 screen_w, u32 screen_h) {
    screen_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                      PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::D32, nullptr);
    readback_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::RGBA16I, nullptr);
    picking_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                       PixelFormat::RGBA16I, nullptr);
}

void WorldRenderer::reset_size(u32 screen_w, u32 screen_h) {
    screen_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                      PixelFormat::RGBA, MSAAType::SAMPLE_COUNT_4,
                      SamplerProperty{}, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::D32, MSAAType::SAMPLE_COUNT_4,
                        SamplerProperty{}, nullptr);
    readback_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                        PixelFormat::RGBA16I, nullptr);
    picking_tex.create(TextureType::TEXTURE_2D, screen_w, screen_h,
                       PixelFormat::RGBA16I, nullptr);
    color_pass.setup(screen_tex, screen_depth, readback_tex);
}

void WorldRenderer::init(Window *window) {
    color_pass.setup(screen_tex, screen_depth, readback_tex);
}
void WorldRenderer::preprocess() {
    fd = {};
    EditorWorld *world = System::gEditor->world_editor.get_current_world();

    if (!world) {
        return;
    }

    FrameGlobal &g_frame = System::gRenderEngine->get_frame_global();
    fd.sky = world->get_sky().sky;

    Camera *cam = &System::gEngine->get_world()->get_camera();
    RHI::UpdateBufferInfo cam_info =
        RHI::alloc_heap(sizeof(Camera::ShaderCamera) * 64);
    Camera::ShaderCamera *matrices = (Camera::ShaderCamera *)cam_info.data;

    cam->fill_shader_camera(matrices);
    RHI::update_from_heap(g_frame.camera, 0, cam_info);

    World *runtime_world = System::gEngine->get_world();
    RHI::UpdateBufferInfo light_info = RHI::alloc_heap(sizeof(STB140Lights));
    std::memset(light_info.data, 0, light_info.size);
    STB140Lights *light_buf = (STB140Lights *)light_info.data;
    runtime_world->get_direction_light().get_stb140(&light_buf->u_dir_light);
    light_buf->u_light_ambient = runtime_world->get_ambient_light();
    for (STB140Light &point_light : light_buf->u_point_lights) {
        point_light.enable = 0.0f;
    }
    RHI::update_from_heap(g_frame.lights, 0, light_info);

    const Frustum &cam_frustum = cam->get_frustum();
    std::vector<f32> depth;
    std::vector<u32> visible_instances;

    EditorTerrain *terrain = world->get_terrain();
    if (terrain == nullptr) return;

    fd.terrain_mesh = terrain->get_mesh();
    Ref<TerrainInstanceData> terrain_instance = terrain->get_instances();
    if (!fd.terrain_mesh.is_null() && !terrain_instance.is_null() &&
        terrain_instance->size() > 0) {
        terrain_instance->upload();
        terrain_instance->frustum_culling(cam_frustum,
                                          fd.terrain_mesh->get_bounding_box(),
                                          visible_instances, depth);
        fd.visible_size = visible_instances.size();
    }

    for (auto &[_, static_model] : world->get_static_models()) {
        if (static_model.model.is_null() || static_model.instances.is_null() ||
            static_model.instances->size() == 0) {
            continue;
        }

        static_model.instances->upload();
        for (Ref<Mesh> mesh : static_model.model->get_meshes()) {
            FrameData::StaticMesh static_mesh;
            static_mesh.mesh = mesh;
            static_mesh.visible_offset = visible_instances.size();
            static_model.instances->frustum_culling(cam_frustum,
                                                    mesh->get_bounding_box(),
                                                    visible_instances, depth);
            static_mesh.visible_size =
                visible_instances.size() - static_mesh.visible_offset;
            if (static_mesh.visible_size > 0) {
                fd.static_meshes.push_back(static_mesh);
            }
        }
    }

    if (!visible_instances.empty()) {
        RHI::update(g_frame.visible, 0, sizeof(u32) * visible_instances.size(),
                    visible_instances.data());
    }
}
void WorldRenderer::_process(RenderCommandDispatcher &dp) {
    color_pass.draw(dp, fd);
    readback_tex->blit_to(ref_cast<Texture>(picking_tex));
}
void WorldRenderer::cleanup() {}

void WorldRenderer::ColorPass::execute(RenderCommandDispatcher &dp, Viewport &,
                                       FrameData &fd) {
    if (!fd.terrain_mesh.is_null() && fd.visible_size > 0) {
        Ref<Material> material = fd.terrain_mesh->get_material();
        RenderDrawDataBuilder mesh_builder = dp.generate_render_data(material);
        u32 visible_offset = 0;
        mesh_builder.push_constant(sizeof(u32), &visible_offset);
        mesh_builder.push_constant(0);
        mesh_builder.bind_vertex_data(fd.terrain_mesh->vertex_data);
        mesh_builder.set_instance(fd.visible_size);
        mesh_builder.bind_index_data(fd.terrain_mesh->lod_indices[0]);
        mesh_builder.set_depth_write(true);
        mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);

        dp.render(mesh_builder, fd.terrain_mesh->get_type(),
                  material->get_pipeline(), 0);
    }

    for (const FrameData::StaticMesh &mesh : fd.static_meshes) {
        if (mesh.visible_size == 0) continue;
        Ref<Material> material = mesh.mesh->get_material();
        RenderDrawDataBuilder mesh_builder = dp.generate_render_data(material);
        mesh_builder.push_constant(mesh.visible_offset);
        mesh_builder.push_constant(0);
        mesh_builder.bind_vertex_data(mesh.mesh->vertex_data);
        mesh_builder.set_instance(mesh.visible_size);
        mesh_builder.bind_index_data(mesh.mesh->lod_indices[0]);
        mesh_builder.set_depth_write(true);
        mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);
        dp.render(mesh_builder, mesh.mesh->get_type(), material->get_pipeline(),
                  0);
    }
    if (!fd.sky.is_null()) {
        RenderDrawDataBuilder sky_builder =
            dp.generate_render_data(ref_cast<Material>(fd.sky->get_material()));
        sky_builder.push_constant(0);
        sky_builder.push_constant(0);
        sky_builder.bind_vertex_data(System::gDefaultStorage->sky_vertices);
        sky_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);
        dp.render(sky_builder, RenderPrimitiveType::TRIANGLES,
                  fd.sky->get_material()->get_pipeline(), 1.0);
    }
}

}  // namespace Seed
