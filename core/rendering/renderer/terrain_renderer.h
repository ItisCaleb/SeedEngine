#ifndef _SEED_TERRAIN_RENDERER_H_
#define _SEED_TERRAIN_RENDERER_H_
#include "renderer.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/terrain.h"
#include <unordered_map>

namespace Seed {
class TerrainRenderer : public Renderer {
    friend RenderEngine;
   private:
    VertexDescription vertices_desc;
    RenderResource model_const_rc;

    RenderResource terrain_shader;

    void init() override;
    void preprocess() override;
    void process(u8 layer) override;
    void cleanup() override;
};
}  // namespace Seed

#endif