#ifndef _SEED_TERRAIN_RENDERER_H_
#define _SEED_TERRAIN_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include "core/rendering/terrain.h"
#include <unordered_map>

namespace Seed {
class TerrainRenderer : public Renderer {
    friend RenderEngine;
   private:
    RenderResource vertices_desc_rc;
    RenderResource model_desc_rc;

    RenderResource terrain_shader;

    void init() override;
    void preprocess() override;
    void process(RenderCommandDispatcher &dp, u64 sort_key) override;
    void cleanup() override;
};
}  // namespace Seed

#endif