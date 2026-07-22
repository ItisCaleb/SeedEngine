#include "renderer.h"
#include "core/system.h"
#include "core/rendering/instance_data.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/rendering/camera.h"
#include "core/rendering/light.h"

namespace Seed {

void FrameGlobal::init() {
    visible = RHI::alloc_storage_buffer(sizeof(int) * 65536,
                                        UpdateFrequence::PERFRAME);
    transform = System::gRenderEngine->get_instance_pool(TRANSFORM_POOL_NAME)
                    ->get_render_buffer();
    terrain = System::gRenderEngine->get_instance_pool(TERRAIN_POOL_NAME)
                  ->get_render_buffer();
    bones = System::gRenderEngine->get_instance_pool(SKELETON_POOL_NAME)
                ->get_render_buffer();
    camera = RHI::alloc_constant(sizeof(Camera::ShaderCamera) * 64,
                                 UpdateFrequence::PERFRAME);
    lights =
        RHI::alloc_constant(sizeof(STB140Lights), UpdateFrequence::PERFRAME);
    csm = RHI::alloc_constant(sizeof(CSMShadow), UpdateFrequence::PERFRAME);
    projection =
        RHI::alloc_constant(sizeof(Mat4), UpdateFrequence::PERFRAME, nullptr);
}

void FrameGlobal::bind(RenderStateDataBuilder &builder) {
    builder.bind_storage_buffer(visible, GlobalBinding::Visible);
    builder.bind_storage_buffer(transform, GlobalBinding::Transform);
    builder.bind_storage_buffer(terrain, GlobalBinding::Terrain);
    builder.bind_storage_buffer(bones, GlobalBinding::Bones);
    builder.bind_constant(camera, GlobalBinding::Camera);
    builder.bind_constant(lights, GlobalBinding::Lights);
    builder.bind_constant(csm, GlobalBinding::CSM);
    builder.bind_constant(projection, GlobalBinding::Projection);
}

}  // namespace Seed
