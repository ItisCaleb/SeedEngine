#ifndef _SEED_MODEL_H_
#define _SEED_MODEL_H_
#include "core/ref.h"
#include "api/render_resource.h"
#include "mesh.h"
#include <vector>

namespace Seed {
class Model : public RefCounted{
    RenderResource vertices_desc_rc;
    RenderResource instance_rc;
    RenderResource instance_desc_rc;

    std::vector<Mesh> meshes;

    static Ref<Model> create();
};
}  // namespace Seed

#endif