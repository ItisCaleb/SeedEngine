#include "model_renderer.h"
#include "editor/editor.h"
#include "core/engine.h"
#include "core/rendering/rhi/render_engine.h"

namespace Seed {
ModelRenderer::ModelRenderer(Ref<Texture> screen_texture,
                                             Ref<Texture> screen_depth) {
    this->screen_tex = screen_texture;
    this->screen_depth = screen_depth;
}

void ModelRenderer::init(Window *window) {
    picking_tex.create(TextureType::TEXTURE_2D, screen_tex->get_width(),
                       screen_depth->get_height(), PixelFormat::RGBA16I,
                       nullptr);
    color_pass.setup(screen_tex, screen_depth, picking_tex);
    visible_ssbo = RHI::alloc_storage_buffer(sizeof(int) * 1024,
                                             UpdateFrequence::PERFRAME);
    mvp = RHI::alloc_constant(sizeof(Mat4) * 2, UpdateFrequence::PERFRAME);

    transform_ssbo = RenderEngine::get_instance()
                       ->get_instance_pool(TRANSFORM_POOL_NAME)
                       ->get_render_buffer();
}
void ModelRenderer::preprocess() {
    EditorModel *model = gEditor->asset_viewer.current_model;

    if (!model) {
        return;
    }
    fd.mesh = model->get_mesh();
    Ref<TerrainInstanceData> instance = model->get_instances();
    AABB bounding_box = fd.mesh->get_bounding_box();
    fd.screen_w = screen_tex->get_width();
    fd.screen_h = screen_tex->get_height();
    /* check instance mesh size > 0 */
    if (instance.is_null() ||
        !instance.is_null() && instance->size() == 0) {
        return;
    }
    Camera *cam = &SeedEngine::get_instance()->get_world()->get_camera();
    Mat4 *matrices = (Mat4 *)RHI::alloc_heap(sizeof(Mat4) * 2);

    matrices[0] = cam->projection_zero();
    matrices[1] = cam->look_at();
    RHI::update_from_heap(mvp, 0, sizeof(Mat4) * 2, matrices);
    const Frustum &cam_frustum = cam->get_frustum();

    /* Use instancing */
    std::vector<f32> depth;
    std::vector<u32> visible_instances;
    instance->frustum_culling(cam_frustum, bounding_box, visible_instances,
                              depth);
    fd.visible_size = visible_instances.size();
    RHI::update(visible_ssbo, 0, sizeof(u32) * visible_instances.size(),
                visible_instances.data());
}
void ModelRenderer::_process(RenderCommandDispatcher &dp) {
    RenderStateDataBuilder builder;
    builder.bind_storage_buffer(visible_ssbo, 0);
    builder.bind_storage_buffer(transform_ssbo, 2);
    builder.bind_constant(mvp, 9);
    dp.set_states(builder);
    color_pass.draw(dp, fd);
}
void ModelRenderer::cleanup() {}

void ModelRenderer::ColorPass::execute(RenderCommandDispatcher &dp,
                                               Viewport &viewport,
                                               FrameData &fd) {
    if (fd.mesh.is_null()) {
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