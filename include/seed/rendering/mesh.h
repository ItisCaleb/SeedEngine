#ifndef _SEED_MESH_H_
#define _SEED_MESH_H_
#include <seed/math/vec2.h>
#include <seed/math/vec3.h>
#include <seed/rendering/texture.h>
#include <seed/ref.h>
#include <vector>

namespace Seed {
struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec2 tex_coord;
};

struct Mesh : public RefCounted {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::vector<Texture> textures;
    u32 VAO, VBO, EBO;

    static Ref<Mesh> create(std::vector<Vertex> &vertices,
                            std::vector<u32> &indices,
                            std::vector<Texture> &textures);
};

}  // namespace Seed

#endif