#include "render_pipeline.h"
#include <spdlog/spdlog.h>

namespace Seed {
RenderPipeline::RenderPipeline(Ref<Shader> shader, VertexDescription *desc, RenderPrimitiveType type,
    const RenderRasterizerState &rst_state,
    const RenderDepthStencilState &depth_state,
    const RenderBlendState &blend_state, VertexDescription *instance_desc)
    :type(type), rst_state(rst_state), depth_state(depth_state), blend_state(blend_state), instance_desc(instance_desc) {
    if (shader.is_null()) {
        throw std::runtime_error("Creating render pipeline with null shader.");
    }
    if(!desc){
        throw std::runtime_error("Creating render pipeline with null vertex description.");
    }
    this->shader = shader;
    this->desc = desc;
}
}  // namespace Seed