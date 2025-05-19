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
        RenderResource model_const_rc;

        void init() override;
        void preprocess() override;
        void process() override;
        void cleanup() override;
        TerrainRenderer(u8 layer) : Renderer(layer) {}
};
}  // namespace Seed

#endif