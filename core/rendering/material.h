#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"
#include "api/render_resource.h"
#include "core/ref.h"

namespace Seed {
struct Material : public RefCounted {
    RenderResource texture_rc;
    RenderResource diffuse_map;
    RenderResource specular_map;
    f32 shiness;

    ~Material();
    static Ref<Material> create(RenderResource texture,
                         RenderResource diffuse_map = {},
                         RenderResource specular_map = {}, f32 shiness = 32.f);
};

}  // namespace Seed

#endif