#ifndef _SEED_TERRAIN_H_
#define _SEED_TERRAIN_H_
#include "core/ref.h"
#include "core/math/vec2.h"
#include "core/resource/material.h"
#include "core/rendering/vertex_data.h"

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
class TerrainRenderer;
class Terrain : public Resource {
        friend TerrainRenderer;

    private:
        u32 width, depth;
        Ref<TerrainMaterial> terrain_mat;
        VertexData vertices;

    public:
        Terrain(u32 width, u32 depth, Ref<Texture> height_map);
        Ref<TerrainMaterial> get_material() { return terrain_mat; }
        VertexData *get_vertices() { return &vertices; }
        ~Terrain();
};

}  // namespace Seed

#endif