#include "terrain.h"
#include "core/math/vec2.h"
#include "core/math/vec4.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/texture.h"
#include "core/types.h"
#include <math.h>
#include <cfloat>

namespace Seed {

#define CHUNK_SIZE (256u)
#define HEIGHT_OFFSET (-128)
#define HEIGHT_SCALE (1)

TerrainMaterial::TerrainMaterial(Ref<Texture> height_map,
                                 Ref<Texture> light_map, Ref<Texture> splat_map,
                                 u32 width, u32 height)
    : Material(DS::get_instance()->terrain_shader) {
    this->set_texture("height_map", height_map);
    if (light_map.is_null()) {
        this->set_texture("terrain_shadowMap",
                          DS::get_instance()->black_texture);
    } else {
        this->set_texture("terrain_shadowMap", light_map);
    }
    this->set_texture("splat_map", splat_map);
    this->raster_state = {.cull_mode = Cullmode::BACK,
                          .patch_control_points = 4};
    this->depth_state = {.depth_mode = DepthMode::OPAQUE};
    this->set_parameter("terrain_size", Vec2{(f32)width, (f32)height});
}
void TerrainMaterial::set_height_map(Ref<Texture> height_map) {
    this->set_texture("height_map", height_map);
}
Ref<Texture> TerrainMaterial::get_height_map() {
    return this->get_texture("height_map");
}

void TerrainMaterial::set_light_map(Ref<Texture> light_map) {
    this->set_texture("terrain_shadowMap", light_map);
}

void TerrainMaterial::set_tex(Ref<Texture> texture) {
    texture->update_sampler(SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                            .wrap_v = SamplerWrap::REPEAT});
    this->set_texture("tex1", texture);
}
void TerrainInstanceData::insert_terrain_data(const TerrainInstance &instance) {
    this->instances.push_back(instance);
}

void TerrainInstanceData::upload() {
    RHI::UpdateBufferInfo instance_info =
        RHI::alloc_heap(sizeof(Vec2) * this->instances.size());
    Vec2 *vecs = (Vec2 *)instance_info.data;
    u32 i = 0;
    for (TerrainInstance &instance : this->instances) {
        memcpy(&vecs[i], &instance, sizeof(Vec2));
        i++;
    }
    _upload(instance_info);
}
void TerrainInstanceData::frustum_culling(const Frustum &frustum,
                                          const AABB &bounding_box,
                                          std::vector<u32> &instance_ids,
                                          std::vector<f32> &depths) {
    u32 i = pool->query(instance_handle).idx;
    for (TerrainInstance &instance : instances) {
        AABB aabb = bounding_box;
        f32 mid_height = (instance.max_height + instance.min_height) / 2.0f;
        aabb.center.x = instance.pos.x;
        aabb.center.z = instance.pos.y;
        aabb.center.y = mid_height;
        aabb.ext.y = instance.max_height - mid_height;
        /* frustum culling */
        if (frustum.within_frustum(aabb)) {
            /* push instance indices */
            instance_ids.push_back(i);
            depths.push_back(frustum.calculate_depth(aabb.center));
        }
        i++;
    }
}
TerrainInstanceData::TerrainInstanceData()
    : InstanceData(
          RenderEngine::get_instance()->get_instance_pool(TERRAIN_POOL_NAME)) {}

void Terrain::create_chunk(Ref<Image> height_map, i32 left, i32 bottom,
                           u32 half_width, u32 half_depth) {
    f32 left_f = (f32)left;
    f32 bottom_f = (f32)bottom;

    std::vector<f32> height_field;
    height_field.resize(CHUNK_SIZE * CHUNK_SIZE);
    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;

    /* start from bottom left to make bounding box right*/
    u8* hm_data = height_map->get_data();
    for (i32 i = CHUNK_SIZE - 1; i >= 0; i--) {
        for (i32 j = 0; j < CHUNK_SIZE; j++) {
            u32 sample_col = j + left + half_width;
            u32 sample_row = i + bottom + half_depth;
            if (sample_col >= height_map->get_width() ||
                sample_row >= height_map->get_height()) {
                height_field[i * CHUNK_SIZE + j] = FLT_MIN;
            } else {
                // get height from y value
                f32 height = (f32)height_map->pixel(sample_col, sample_row)[1] *
                                 HEIGHT_SCALE +
                             HEIGHT_OFFSET;
                max_height = std::max(max_height, height);
                min_height = std::min(min_height, height);
                height_field[i * CHUNK_SIZE + j] = height;
            }
        }
    }

    // empty chunk
    if (max_height == min_height) return;

    // postion center
    // uv bottom left
    this->instances->insert_terrain_data(TerrainInstance{
        .pos = Vec2{left_f + CHUNK_SIZE / 2, bottom_f + CHUNK_SIZE / 2},
        .max_height = max_height,
        .min_height = min_height});

    PhysicBody &body = this->bodies.emplace_back();
    PhysicHeightmapShape shape(
        height_field.data(), CHUNK_SIZE,
        Vec3{-(i32)CHUNK_SIZE / 2, 0, -(i32)CHUNK_SIZE / 2});

    PhysicEngine::get_instance()->create_body(
        body, shape, PhysicBodyType::STATIC,
        Vec3{left_f + CHUNK_SIZE / 2, 0, bottom_f + CHUNK_SIZE / 2});
}

void Terrain::build_mesh() {
    i32 half_chunk = CHUNK_SIZE / 2;
    u32 chunk_cnt = 16;
    u32 vertex_row_cnt = chunk_cnt + 1;
    std::vector<TerrainVertex> vertices;
    f32 offset = CHUNK_SIZE / chunk_cnt;
    for (i32 i = 0; i < vertex_row_cnt; i++) {
        for (i32 j = 0; j < vertex_row_cnt; j++) {
            vertices.push_back(TerrainVertex{Vec2{
                offset * i - CHUNK_SIZE / 2, offset * j - CHUNK_SIZE / 2}});
        }
    }

    u32 step = chunk_cnt / chunk_cnt;
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

Terrain::Terrain(Ref<Image> height_map, Ref<Texture> light_map,
                 Ref<Texture> splat_map) {
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

    Ref<Texture> height_map_tex = height_map->create_texture();
    terrain_mat.create(height_map_tex, light_map, splat_map, width, depth);
    this->instances.create();

    build_mesh();
    this->mesh->set_material(ref_cast<Material>(terrain_mat));

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

Terrain::~Terrain() {}
}  // namespace Seed