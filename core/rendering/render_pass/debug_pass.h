#ifndef _SEED_DEBUG_PASS_H_
#define _SEED_DEBUG_PASS_H_
#include "render_pass.h"
#include "core/rendering/api/render_resource.h"
#include <vector>
#include "core/rendering/model.h"

namespace Seed {
class DebugPass : public RenderPass {
    friend RenderEngine;

   private:
    std::vector<AABB> entity_aabb;
    RenderResource aabb_vertices_rc;
    RenderResource aabb_desc_rc;
    void init() override;
    void process(RenderCommandDispatcher &dp, u64 sort_key) override;
};
}  // namespace Seed

#endif