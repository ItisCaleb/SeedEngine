#include "terrain.h"

namespace Seed {
Terrain::Terrain(u32 width, u32 depth, Ref<Texture> height_map)
    : width(width), depth(depth), height_map(height_map) {
    std::vector<TerrainVertex> vertices;
    f32 left = -(width / 2.0f);
    f32 top = -(depth / 2.0f);
    i32 rez = 20;
    f32 rezf = (f32)rez;
    for (i32 i = 0; i < rez; i++) {
        for (i32 j = 0; j < rez; j++) {
            vertices.emplace_back(TerrainVertex{
                Vec3{left + width * i / rezf, 0.0f, top + depth * j / rezf},
                Vec2{i / rezf, j / rezf}});
            vertices.emplace_back(TerrainVertex{
                Vec3{left + width * (i+1) / rezf, 0.0f, top + depth * j / rezf},
                Vec2{(i + 1) / rezf, j / rezf}});
            vertices.emplace_back(TerrainVertex{
                Vec3{left + width * i / rezf, 0.0f, top + depth * (j + 1) / rezf},
                Vec2{i / rezf, (j + 1) / rezf}});
            vertices.emplace_back(TerrainVertex{
                Vec3{left + width * (i + 1) / rezf, 0.0f, top + depth * (j + 1) / rezf},
                Vec2{(i + 1) / rezf, (j + 1) / rezf}});
        }
    }
    this->vertices.alloc_vertex(sizeof(TerrainVertex), vertices.size(), vertices.data());
}

Terrain::~Terrain() { this->vertices.dealloc(); }
}  // namespace Seed