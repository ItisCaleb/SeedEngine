#include "terrain.h"

namespace Seed {
Terrain::Terrain(u32 width, u32 depth, Ref<Texture> height_map)
    : width(width), depth(depth) {
    std::vector<TerrainVertex> vertices;
    f32 left = -(width / 2.0f);
    f32 top = -(depth / 2.0f);
    i32 rez = 20;
    f32 rezf = (f32)rez;
    for (i32 i = 0; i < rez; i++) {
        for (i32 j = 0; j < rez; j++) {
            vertices.emplace_back(TerrainVertex{
                Vec2{left + width * i / rezf, top + depth * j / rezf},
                Vec2{i / rezf, j / rezf}});
            vertices.emplace_back(TerrainVertex{
                Vec2{left + width * (i + 1) / rezf, top + depth * j / rezf},
                Vec2{(i + 1) / rezf, j / rezf}});
            vertices.emplace_back(TerrainVertex{
                Vec2{left + width * i / rezf, top + depth * (j + 1) / rezf},
                Vec2{i / rezf, (j + 1) / rezf}});
            vertices.emplace_back(
                TerrainVertex{Vec2{left + width * (i + 1) / rezf,
                                   top + depth * (j + 1) / rezf},
                              Vec2{(i + 1) / rezf, (j + 1) / rezf}});
        }
    }
    RenderResource vertices_rc;
    vertices_rc.alloc_vertex(sizeof(TerrainVertex), vertices.size(),
                             vertices.data());
    this->vertices.bind_vertices(sizeof(TerrainVertex), vertices.size(),
                                 vertices_rc);
    terrain_mat.create();
    terrain_mat->set_texture_map(Material::DIFFUSE, height_map);
}

Terrain::~Terrain() {}
}  // namespace Seed