#include "terrain.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
#include "core/concurrency/thread_pool.h"
#include <math.h>

namespace Seed {

#define CHUNK_SIZE (256u)

TerrainMaterial::TerrainMaterial(Ref<Texture> height_map)
    : Material(DS::get_instance()->terrain_shader) {
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

TerrainChunk::TerrainChunk(Ref<Image> height_map, i32 left, i32 top,
                           u32 half_width, u32 half_depth, f32 tex_x_stride,
                           f32 tex_y_stride) {
    f32 tex_left = (left + half_width) / (f32)height_map->get_width();
    f32 tex_top = (top + half_depth) / (f32)height_map->get_height();
    f32 left_f = (f32)left;
    f32 top_f = (f32)top;
    /* top left */
    this->vertices[0] =
        TerrainVertex{Vec2{left_f, top_f}, Vec2{tex_left, tex_top}};

    /* top right */
    this->vertices[1] = TerrainVertex{Vec2{left_f + CHUNK_SIZE, top_f},
                                      Vec2{tex_left + tex_x_stride, tex_top}};

    /* bottom left */
    this->vertices[2] = TerrainVertex{Vec2{left_f, top_f + CHUNK_SIZE},
                                      Vec2{tex_left, tex_top + tex_y_stride}};

    /* bottom right */
    this->vertices[3] =
        TerrainVertex{Vec2{left_f + CHUNK_SIZE, top_f + CHUNK_SIZE},
                      Vec2{tex_left + tex_x_stride, tex_top + tex_y_stride}};
    
    for (i32 i = 0; i < 16; i++) {
        for (i32 j = 0; j < 16; j++) {
        }
    }

    std::vector<u32> indices;
    for (i32 i = 0; i < 4; i++) {
        indices.push_back(i);
    }

    std::vector<f32> height_field;
    height_field.resize(CHUNK_SIZE * CHUNK_SIZE);
    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;
    for (i32 i = 0; i < CHUNK_SIZE; i++) {
        for (i32 j = 0; j < CHUNK_SIZE; j++) {
            u32 sample_col = i + top + half_depth;
            u32 sample_row = j + left + half_width;
            if (sample_col >= height_map->get_height() ||
                sample_row >= height_map->get_width()) {
                height_field[i * CHUNK_SIZE + j] = FLT_MIN;
            } else {
                // get height from y value
                f32 height =
                    (f32)height_map
                        ->get_data()[(sample_col * height_map->get_width() +
                                      sample_row) *
                                         4 +
                                     1];
                max_height = std::max(max_height, height);
                min_height = std::min(min_height, height);
                height_field[i * CHUNK_SIZE + j] = height;
            }
        }
    }
    f32 center_h = (max_height + min_height) / 2.0f;
    AABB bounding_box = AABB{
        .center =
            Vec3{left_f + CHUNK_SIZE / 2, center_h, top_f + CHUNK_SIZE / 2},
        .ext = Vec3{CHUNK_SIZE / 2, max_height - center_h, CHUNK_SIZE / 2}};

    // this->mesh.create(&DS::get_instance()->terrain_desc, this->vertices, indices, bounding_box);
    // this->mesh->set_type(RenderPrimitiveType::PATCHES);


    PhysicHeightmapShape shape(
        height_field.data(), CHUNK_SIZE,
        Vec3{-(i32)CHUNK_SIZE / 2, -16, -(i32)CHUNK_SIZE / 2},
        Vec3{1, 0.25f, 1});

    PhysicEngine::get_instance()->create_body(
        this->body, shape, PhysicBodyType::STATIC,
        Vec3{left_f + CHUNK_SIZE / 2, 0, top_f + CHUNK_SIZE / 2});
}

Terrain::Terrain(Ref<Image> height_map) {
    u32 hmap_width = height_map->get_width();
    u32 hmap_height = height_map->get_height();

    this->width = ((hmap_width + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1));
    this->depth = ((hmap_height + CHUNK_SIZE - 1) & ~(CHUNK_SIZE - 1));

    i32 left = -(width / 2);
    i32 top = -(depth / 2);
    u32 row_size = this->width / CHUNK_SIZE;
    u32 col_size = this->depth / CHUNK_SIZE;
    f32 tex_x_stride = (f32)CHUNK_SIZE / hmap_width;
    f32 tex_y_stride = (f32)CHUNK_SIZE / hmap_height;
    u32 half_width = this->width / 2;
    u32 half_depth = this->depth / 2;

    Ref<Texture> height_map_tex = height_map->create_texture();
    terrain_mat.create(height_map_tex);

    ThreadPool *pool = ThreadPool::get_instance();
    pool->add_work([=](void *) {
        // start from bottom left
        this->chunks.reserve(col_size * row_size);
        for (i32 i = 0; i < col_size; i++) {
            for (i32 j = 0; j < row_size; j++) {
                i32 l = left + (i32)CHUNK_SIZE * j;
                i32 t = top + (i32)CHUNK_SIZE * i;
                TerrainChunk &chunk = this->chunks.emplace_back(
                    height_map, l, t, half_width, half_depth, tex_x_stride,
                    tex_y_stride);
                //chunk.mesh->set_material(terrain_mat);
            }
        }
        this->loaded = true;
    });
}

Terrain::~Terrain() {}
}  // namespace Seed