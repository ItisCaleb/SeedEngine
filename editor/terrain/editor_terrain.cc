#include "editor_terrain.h"
#include "core/rendering/mesh_storage.h"
#include "editor/editor_storage.h"

namespace Seed {

#define CHUNK_SIZE (256u)
#define HEIGHT_OFFSET (-128)
#define HEIGHT_SCALE (1)

EditorTerrainMaterial::EditorTerrainMaterial(Ref<Texture> height_map)
    : Material(ES::get_instance()->editor_terrain_shader) {
    this->set_texture("height_map", height_map);
    this->raster_state = {.cull_mode = Cullmode::FRONT,
                          .patch_control_points = 4};
}

Ref<Texture> EditorTerrainMaterial::get_height_map() {
    return this->get_texture("height_map");
}

void EditorTerrain::build_mesh() {
    f32 tex_x_stride = (f32)CHUNK_SIZE / hmap_width;
    f32 tex_y_stride = (f32)CHUNK_SIZE / hmap_height;
    u32 vertex_row_cnt = 5;
    u32 chunk_cnt = 4;
    u32 step = (vertex_row_cnt - 1) / chunk_cnt;
    std::vector<TerrainVertex> vertices;
    f32 offset = CHUNK_SIZE / 4;
    for (i32 i = 0; i < vertex_row_cnt; i++) {
        for (i32 j = 0; j < vertex_row_cnt; j++) {
            vertices.push_back(TerrainVertex{
                Vec2{offset * j - CHUNK_SIZE / 2, offset * i - CHUNK_SIZE / 2},
                Vec2{(tex_x_stride / (f32)(vertex_row_cnt - 1)) * j,
                     (tex_y_stride / (f32)(vertex_row_cnt - 1)) * i}});
        }
    }

    std::vector<u32> indices;
    for (i32 i = 0; i < chunk_cnt; i++) {
        for (i32 j = 0; j < chunk_cnt; j++) {
            i32 chunk_offset = j * step + i * step * vertex_row_cnt;
            /* top left */
            indices.push_back(chunk_offset);
            /* top right */
            indices.push_back(chunk_offset + step);
            /* bottom left */
            indices.push_back(chunk_offset + vertex_row_cnt * step);
            /* bottom right */
            indices.push_back(chunk_offset + vertex_row_cnt * step + step);
        }
    }

    this->mesh.create(&DS::get_instance()->terrain_desc, vertices, indices,
                      AABB{.center = Vec3{0, 0, 0},
                           .ext = Vec3{CHUNK_SIZE / 2, 0, CHUNK_SIZE / 2}});
    this->mesh->set_type(RenderPrimitiveType::PATCHES);
}
void EditorTerrain::create_chunk(Ref<Image> height_map, i32 left, i32 bottom,
                                 u32 half_width, u32 half_depth) {
    f32 left_f = (f32)left;
    f32 bottom_f = (f32)bottom;
    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;

    auto &hm_data = height_map->get_data();
    for (i32 i = CHUNK_SIZE - 1; i >= 0; i--) {
        for (i32 j = 0; j < CHUNK_SIZE; j++) {
            u32 sample_col = j + left + half_width;
            u32 sample_row = i + bottom + half_depth;
            if (sample_col < height_map->get_width() &&
                sample_row < height_map->get_height()) {
                // get height from y value
                f32 height = (f32)height_map->pixel(sample_col, sample_row)[1] *
                                 HEIGHT_SCALE +
                             HEIGHT_OFFSET;
                max_height = std::max(max_height, height);
                min_height = std::min(min_height, height);
            }
        }
    }

    // empty chunk
    if (max_height == min_height) return;
    f32 u = (left + half_width) / (f32)hmap_width;
    f32 v = (bottom + half_depth) / (f32)hmap_height;

    // postion center
    // uv bottom left
    this->instances->insert_terrain_data(TerrainInstance{
        .pos = Vec2{left_f + CHUNK_SIZE / 2, bottom_f + CHUNK_SIZE / 2},
        .tex_coord = Vec2{u, v},
        .max_height = max_height,
        .min_height = min_height});
}
EditorTerrain::EditorTerrain(Ref<Image> height_map) {
    this->hmap_width = height_map->get_width();
    this->hmap_height = height_map->get_height();

    this->width = ((hmap_width + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1));
    this->depth = ((hmap_height + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1));

    i32 left = -(width / 2);
    i32 bottom = -(depth / 2);
    u32 row_size = this->width / CHUNK_SIZE;
    u32 col_size = this->depth / CHUNK_SIZE;
    u32 half_width = this->width / 2;
    u32 half_depth = this->depth / 2;

    heightmap_texture = height_map->create_mappable_texture();
    material.create(ref_cast<Texture>(heightmap_texture));
    this->instances.create();

    build_mesh();
    this->mesh->set_material(ref_cast<Material>(material));

    // start from bottom left
    for (i32 i = 0; i < col_size; i++) {
        for (i32 j = 0; j < row_size; j++) {
            i32 l = left + (i32)CHUNK_SIZE * j;
            i32 b = bottom + (i32)CHUNK_SIZE * i;
            create_chunk(height_map, l, b, half_width, half_depth);
        }
    }
    this->instances->upload();
}
EditorTerrain::~EditorTerrain() {
    
}
}  // namespace Seed