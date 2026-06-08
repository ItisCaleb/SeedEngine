#include "editor_terrain.h"
#include <fmt/format.h>
#include "core/ref.h"
#include "core/rendering/render_common.h"
#include "core/resource/image.h"
#include "core/resource/texture.h"
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
    u32 vertex_row_cnt = 5;
    u32 chunk_cnt = 4;
    u32 step = (vertex_row_cnt - 1) / chunk_cnt;
    std::vector<TerrainVertex> vertices;
    f32 offset = CHUNK_SIZE / chunk_cnt;
    for (i32 i = 0; i < vertex_row_cnt; i++) {
        for (i32 j = 0; j < vertex_row_cnt; j++) {
            vertices.push_back(TerrainVertex{Vec2{
                offset * j - CHUNK_SIZE / 2, offset * i - CHUNK_SIZE / 2}});
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

EditorTerrain::EditorTerrain() {
    heightmaps.create(TextureType::TEXTURE_2D_ARRAY, CHUNK_SIZE + 1, CHUNK_SIZE + 1,
                      256, PixelFormat::RG, SamplerProperty{});
    material.create(ref_cast<Texture>(heightmaps));

    this->instances.create();

    build_mesh();
    this->mesh->set_material(ref_cast<Material>(material));
}

void EditorTerrain::add_chunk(i32 x, i32 y, Ref<Image> height_map) {
    heightmaps->update_layer(CHUNK_SIZE + 1, CHUNK_SIZE + 1, last_heightmap,
                             height_map->get_data());

    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;

    for (i32 i = 0; i <= CHUNK_SIZE; i++) {
        for (i32 j = 0; j <= CHUNK_SIZE; j++) {
            // get height from y value
            f32 height =
                (f32)height_map->pixel(i, j)[1] * HEIGHT_SCALE + HEIGHT_OFFSET;
            max_height = std::max(max_height, height);
            min_height = std::min(min_height, height);
        }
    }

    instances->insert_terrain_data(TerrainInstance{
        .pos = Vec2{(f32)(x * CHUNK_SIZE), (f32)(y * CHUNK_SIZE)},
        .heightmap_index = last_heightmap,
        .max_height = max_height,
        .min_height = min_height});
    last_heightmap++;
}

// void EditorTerrain::gen_lightmap() {
//     Ref<Image> terrain_shadow_map(PixelFormat::RGBA, this->hmap_width,
//                                   this->hmap_height);

//     auto get_height = [&](Vec2 p) {
//         return heightmap_texture->pixel(p.x, p.y)[1];
//     };

//     auto angle = [&](Vec2 p, Vec2 q) {
//         f32 dist_sqr = (q - p).length_sqr();
//         f32 ph = get_height(p);
//         f32 qh = get_height(q);
//         f32 dh = qh - ph;
//         f32 angle = dh / sqrt(dh * dh + dist_sqr);
//         return angle;
//     };

//     auto slope = [&](Vec2 p, Vec2 q) {
//         f32 dist = (q - p).length();
//         f32 ph = get_height(p);
//         f32 qh = get_height(q);
//         f32 dh = qh - ph;
//         f32 slope = dh / dist;
//         return slope;
//     };
//     auto calculate_lightmap = [&](Vec3 light_dir, u32 light_map_channel) {
//         std::vector<Vec2> starts;

//         if (light_dir.x > 0) {
//             for (int z = 0; z < this->hmap_height; ++z)
//                 starts.push_back({0.0f, (float)z});
//         } else if (light_dir.x < 0) {
//             for (int z = 0; z < this->hmap_height; ++z)
//                 starts.push_back({(float)(hmap_width - 1), (float)z});
//         }

//         if (light_dir.z > 0) {
//             for (int x = 0; x < this->hmap_width; ++x)
//                 starts.push_back({(float)x, 0.0f});
//         } else if (light_dir.z < 0) {
//             for (int x = 0; x < this->hmap_width; ++x)
//                 starts.push_back({(float)x, (float)(this->hmap_height - 1)});
//         }
//         std::deque<Vec2> hull;

//         for (Vec2 &sp : starts) {
//             f32 row = sp.y;
//             f32 col = sp.x;
//             row += light_dir.z;
//             col += light_dir.x;
//             i32 irow = row;
//             i32 icol = col;
//             hull.clear();
//             while (icol >= 0 && icol < this->hmap_width && irow >= 0 &&
//                    irow < this->hmap_height) {
//                 Vec2 cur = Vec2{col, row};
//                 while (hull.size() >= 2 &&
//                        slope(cur, hull[0]) < slope(cur, hull[1])) {
//                     hull.pop_front();
//                 }
//                 if (hull.empty())
//                     terrain_shadow_map->pixel(icol, irow)[light_map_channel]
//                     =
//                         0;
//                 else {
//                     f32 s = slope(cur, hull[0]);
//                     terrain_shadow_map->pixel(icol, irow)[light_map_channel]
//                     =
//                         s > 0 ? 255 * angle(cur, hull[0]) : 0;
//                 }

//                 while (!hull.empty() && get_height(cur) >
//                 get_height(hull[0])) {
//                     hull.pop_front();
//                 }
//                 hull.push_front(cur);

//                 col += light_dir.x;
//                 row += light_dir.z;
//                 icol = col;
//                 irow = row;
//             }
//         }
//     };
//     calculate_lightmap(Vec3{1, 0, 0}, 0);
//     calculate_lightmap(Vec3{-1, 0, 0}, 1);
//     calculate_lightmap(Vec3{0, 0, 1}, 2);
//     calculate_lightmap(Vec3{0, 0, -1}, 3);

//     this->light_map =
//         terrain_shadow_map->median_filter(7,
//         true)->create_mappable_texture();
//     light_map_generated = true;
//     material->set_light_map(ref_cast<Texture>(this->light_map));
// }

EditorTerrain::~EditorTerrain() {}
}  // namespace Seed
