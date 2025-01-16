#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include "core/math/vec2.h"
#include "core/math/vec3.h"
#include "texture.h"
#include "core/ref.h"
#include <vector>

namespace Seed {
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 tex_coord;
};

struct Mesh : public RefCounted {
    std::vector<Vertex> vertices;
    std::vector<Texture> textures;

    static Ref<Mesh> create(std::vector<Vertex> &vertices,
                            std::vector<Texture> &textures);
};

}  // namespace Seed

#endif