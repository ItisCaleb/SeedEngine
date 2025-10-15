#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/math/vec2.h"
#include "core/resource/material.h"
#include "core/rendering/vertex_data.h"
#include "core/resource/image.h"
#include "core/physic/physic_body.h"
#include "core/collision/aabb.h"
#include <vector>
#include "core/rendering/mesh.h"

namespace Seed {

struct TerrainVertex {
        Vec2 pos;
        Vec2 tex_coord;
};
class TerrainMaterial : public Material {
    public:
        TerrainMaterial(Ref<Texture> height_map);
        void set_height_map(Ref<Texture> height_map);
        Ref<Texture> get_height_map();
};

class Terrain;
class DefaultRenderer;
class TerrainChunk {
        friend DefaultRenderer;
        friend Terrain;

    private:
        TerrainVertex vertices[4];
        Ref<Mesh> mesh;
        PhysicBody body;

    public:
        TerrainChunk(Ref<Image> height_map, i32 left, i32 bottom, u32 half_width,
                     u32 half_depth, f32 tex_x_stride, f32 tex_y_stride);
};

class Terrain : public Resource {
    private:
        u32 width, depth;
        Vec3 position;
        Ref<TerrainMaterial> terrain_mat;
        std::vector<TerrainChunk> chunks;
        bool loaded = false;

    public:
        Terrain(Ref<Image> height_map);
        Ref<TerrainMaterial> get_material() { return terrain_mat; }
        std::vector<TerrainChunk> &get_chunks() { return chunks; };
        bool is_loaded(){return loaded;}

        ~Terrain();
};

}  // namespace Seed

#endif