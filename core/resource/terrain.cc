#include "terrain.h"
#include "core/resource/default_storage.h"

namespace Seed {

TerrainMaterial::TerrainMaterial(Ref<Texture> height_map)
    : Material(DS::get_instance()->get_terrain_shader()) {
    this->add_texture_unit(height_map);
    this->raster_state = {.patch_control_points = 4};
    this->depth_state = {.depth_on = true};
}
void TerrainMaterial::set_height_map(Ref<Texture> height_map) {
    this->set_texture_unit(0, height_map);
}
Ref<Texture> TerrainMaterial::get_height_map() {
    return this->get_texture_unit(0)->get_texture();
}

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
    terrain_mat.create(height_map);
}

Terrain::~Terrain() {}
}  // namespace Seed