#include "terrain.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
#include "core/concurrency/thread_pool.h"
#include "core/rendering/api/render_engine.h"
#include <math.h>
#include <deque>
#include <cfloat>

namespace Seed {

#define CHUNK_SIZE (256u)
#define HEIGHT_OFFSET (-128)
#define HEIGHT_SCALE (1)

TerrainMaterial::TerrainMaterial(Ref<Texture> height_map)
    : Material(DS::get_instance()->terrain_shader) {
    this->shadow_pipeline = DS::get_instance()->shadow_map_terrain_pipeline;
    this->set_texture("height_map", height_map);
    this->set_texture("terrain_shadowMap", DS::get_instance()->black_texture);
    this->raster_state = {.cull_mode = Cullmode::FRONT,
                          .patch_control_points = 4};
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

void TerrainInstanceData::insert_terrain_data(const TerrainInstance &instance) {
    this->instances.push_back(instance);
}

void TerrainInstanceData::upload() {
    if (instance_handle == NULL_HANDLE) {
        instance_handle = pool->alloc(this->instances.size());
    } else {
        auto block = pool->query(instance_handle);
        if (block.size < this->instances.size()) {
            pool->free(instance_handle);
            instance_handle = pool->alloc(this->instances.size());
        }
    }
    /* upload */
    InstanceDataPool::Block block = pool->query(instance_handle);
    Vec4 *vecs = (Vec4 *)RHI::alloc_heap(sizeof(Vec4) * this->instances.size());
    u32 i = 0;
    for (TerrainInstance &instance : this->instances) {
        memcpy(&vecs[i], &instance, sizeof(Vec4));
        i++;
    }
    RHI::update_from_heap(pool->get_render_buffer(), sizeof(Vec4) * block.idx,
                          sizeof(Vec4) * this->instances.size(), vecs);
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
          RenderEngine::get_instance()->get_instance_pool("TerrainDataPool")) {}

void Terrain::create_chunk(Ref<Image> height_map, i32 left, i32 bottom,
                           u32 half_width, u32 half_depth) {
    f32 left_f = (f32)left;
    f32 bottom_f = (f32)bottom;

    std::vector<f32> height_field;
    height_field.resize(CHUNK_SIZE * CHUNK_SIZE);
    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;

    /* start from bottom left to make bounding box right*/
    auto &hm_data = height_map->get_data();
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
    f32 u = (left + half_width) / (f32)hmap_width;
    f32 v = (bottom + half_depth) / (f32)hmap_height;

    // postion center
    // uv bottom left
    this->instances->insert_terrain_data(TerrainInstance{
        .pos = Vec2{left_f + CHUNK_SIZE / 2, bottom_f + CHUNK_SIZE / 2},
        .tex_coord = Vec2{u, v},
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

void Terrain::gen_lightmap(Ref<Image> height_map) {
    Ref<Image> terrain_shadow_map(PixelFormat::R, this->hmap_width,
                                  this->hmap_height);

    Vec3 light_dir = Vec3{-0.5, -0.5, 0}.norm();
    std::vector<Vec2> starts;

    if (light_dir.x > 0) {
        for (int z = 0; z < this->hmap_height; ++z)
            starts.push_back({0.0f, (float)z});
    } else if (light_dir.x < 0) {
        for (int z = 0; z < this->hmap_height; ++z)
            starts.push_back({(float)(hmap_width - 1), (float)z});
    }

    if (light_dir.z > 0) {
        for (int x = 0; x < this->hmap_width; ++x)
            starts.push_back({(float)x, 0.0f});
    } else if (light_dir.z < 0) {
        for (int x = 0; x < this->hmap_width; ++x)
            starts.push_back({(float)x, (float)(this->hmap_height - 1)});
    }
    for (Vec2 &sp : starts) {
        f32 row = sp.y;
        f32 col = sp.x;
        row -= light_dir.z;
        col += light_dir.x;
        i32 irow = row;
        i32 icol = col;
        std::deque<Vec2> hull;

        auto get_height = [&](Vec2 p) {
            return height_map->pixel(p.x, p.y)[0];
        };

        auto angle = [&](Vec2 p, Vec2 q) {
            f32 dist_sqr = (q - p).length_sqr();
            f32 ph = get_height(p);
            f32 qh = get_height(q);
            f32 dh = qh - ph;
            f32 angle = dh / sqrt(dh * dh + dist_sqr);
            return angle;
        };

        auto slope = [&](Vec2 p, Vec2 q) {
            f32 dist = (q - p).length();
            f32 ph = get_height(p);
            f32 qh = get_height(q);
            f32 dh = qh - ph;
            f32 slope = dh / dist;
            return slope;
        };

        while (icol >= 0 && icol < this->hmap_width && irow >= 0 &&
               irow < this->hmap_height) {
            Vec2 cur = Vec2{col, row};
            while (hull.size() >= 2 &&
                   slope(cur, hull[0]) < slope(cur, hull[1])) {
                hull.pop_front();
            }
            if (hull.empty())
                terrain_shadow_map->pixel(icol, irow)[0] = 0;
            else {
                f32 s = slope(cur, hull[0]);
                terrain_shadow_map->pixel(icol, irow)[0] =
                    s > 0 ? 255 * angle(cur, hull[0]) : 0;
            }

            while (!hull.empty() && get_height(cur) > get_height(hull[0])) {
                hull.pop_front();
            }
            hull.push_front(cur);

            col += light_dir.x;
            row -= light_dir.z;
            icol = col;
            irow = row;
        }
    }

    Ref<Texture> shadow_map(TextureType::TEXTURE_2D, this->hmap_width,
                            this->hmap_height, PixelFormat::R, nullptr);
    terrain_shadow_map->median_filter(7)->upload(shadow_map);
    terrain_mat->set_light_map(shadow_map);
}

Terrain::Terrain(Ref<Image> height_map) {
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
    terrain_mat.create(height_map_tex);
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
    MeshStorage::get_instance()->add_mesh(
        this->mesh, ref_cast<InstanceData>(this->instances));
    gen_lightmap(height_map);
}

Terrain::~Terrain() {}
}  // namespace Seed