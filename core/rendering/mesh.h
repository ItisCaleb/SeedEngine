#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "core/ref.h"
#include "api/render_resource.h"
#include "material.h"
#include <vector>

namespace Seed {
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 tex_coord;
};

struct Mesh{
    RenderResource vertices_rc;
    RenderResource indices_rc;

    ~Mesh();
    void create(std::vector<Vertex> &vertices,
                            std::vector<u32> &indices);
};

}  // namespace Seed

#endif