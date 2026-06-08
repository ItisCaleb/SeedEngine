#include "world_renderer.h"
#include <cstring>
#include "editor/editor.h"
#include "core/engine.h"
#include "core/rendering/light.h"
#include "core/rendering/rhi/render_engine.h"

namespace Seed {
WorldRenderer::WorldRenderer(Ref<Texture> screen_texture,
                             Ref<Texture> screen_depth,
                             Ref<Texture> picking_texture) {
    this->screen_tex = screen_texture;
    this->screen_depth = screen_depth;
    this->picking_tex = picking_texture;
}

void WorldRenderer::rebind_textures(Ref<Texture> screen_texture,
                                    Ref<Texture> screen_depth,
                                    Ref<Texture> picking_texture) {
    this->screen_tex = screen_texture;
    this->screen_depth = screen_depth;
    this->picking_tex = picking_texture;
    color_pass.setup(screen_tex, screen_depth, picking_tex);
}

void WorldRenderer::init(Window *window) {
    color_pass.setup(screen_tex, screen_depth, picking_tex);
    visible_ssbo = RHI::alloc_storage_buffer(sizeof(int) * 1024,
                                             UpdateFrequence::PERFRAME);
    camera = RHI::alloc_constant(sizeof(Mat4) * 64, UpdateFrequence::PERFRAME);
    lights =
        RHI::alloc_constant(sizeof(STB140Lights), UpdateFrequence::PERFRAME);

    terrain_ssbo = RenderEngine::get_instance()
                       ->get_instance_pool(TERRAIN_POOL_NAME)
                       ->get_render_buffer();
}
void WorldRenderer::preprocess() {
    fd = {};
    EditorWorld *world = gEditor->world_editor.get_current_world();

    if (!world) {
        return;
    }

    fd.mesh = world->terrain->get_mesh();
    Ref<TerrainInstanceData> instance = world->terrain->get_instances();
    AABB bounding_box = fd.mesh->get_bounding_box();
    fd.screen_w = screen_tex->get_width();
    fd.screen_h = screen_tex->get_height();
    /* check instance mesh size > 0 */
    if (instance.is_null() || !instance.is_null() && instance->size() == 0) {
        return;
    }
    instance->upload();

    Camera *cam = &SeedEngine::get_instance()->get_world()->get_camera();
    RHI::UpdateBufferInfo cam_info =
        RHI::alloc_heap(sizeof(Camera::ShaderCamera) * 64);
    Camera::ShaderCamera *matrices = (Camera::ShaderCamera *)cam_info.data;

    cam->fill_shader_camera(matrices);
    RHI::update_from_heap(camera, 0, cam_info);

    World *runtime_world = SeedEngine::get_instance()->get_world();
    RHI::UpdateBufferInfo light_info = RHI::alloc_heap(sizeof(STB140Lights));
    std::memset(light_info.data, 0, light_info.size);
    STB140Lights *light_buf = (STB140Lights *)light_info.data;
    runtime_world->get_direction_light().get_stb140(&light_buf->u_dir_light);
    light_buf->u_light_ambient = runtime_world->get_ambient_light();
    for (STB140Light &point_light : light_buf->u_point_lights) {
        point_light.enable = 0.0f;
    }
    RHI::update_from_heap(lights, 0, light_info);

    const Frustum &cam_frustum = cam->get_frustum();

    /* Use instancing */
    std::vector<f32> depth;
    std::vector<u32> visible_instances;
    instance->frustum_culling(cam_frustum, bounding_box, visible_instances,
                              depth);
    fd.visible_size = visible_instances.size();
    if (visible_instances.empty()) return;
    RHI::update(visible_ssbo, 0, sizeof(u32) * visible_instances.size(),
                visible_instances.data());
}
void WorldRenderer::_process(RenderCommandDispatcher &dp) {
    if (fd.mesh.is_null()) {
        return;
    }

    RenderStateDataBuilder builder;
    builder.bind_storage_buffer(visible_ssbo, 0);
    builder.bind_storage_buffer(terrain_ssbo, 2);
    builder.bind_constant(camera, 8);
    builder.bind_constant(lights, 10);
    dp.set_states(builder);
    color_pass.draw(dp, fd);
}
void WorldRenderer::cleanup() {}

void WorldRenderer::ColorPass::execute(RenderCommandDispatcher &dp,
                                       Viewport &viewport, FrameData &fd) {
    if (fd.mesh.is_null() || fd.visible_size == 0) {
        return;
    }
    Ref<Material> material = fd.mesh->get_material();
    RenderDrawDataBuilder mesh_builder = dp.generate_render_data(material);
    u32 visible_offset = 0;
    mesh_builder.push_constant(sizeof(u32), &visible_offset);
    mesh_builder.bind_vertex_data(fd.mesh->vertex_data);
    mesh_builder.set_instance(fd.visible_size);
    mesh_builder.bind_index_data(fd.mesh->lod_indices[0]);
    mesh_builder.set_depth_write(true);
    mesh_builder.set_depth_test(CompareOP::LESS_OR_EQUAL);

    dp.render(mesh_builder, fd.mesh->get_type(), material->get_pipeline(), 0);
}

}  // namespace Seed
