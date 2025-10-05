#include "terrain.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
#include "core/concurrency/thread_pool.h"

namespace Seed {

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

Terrain::Terrain(u32 width, u32 depth, Ref<Image> height_map)
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

    ThreadPool *pool = ThreadPool::get_instance();
    pool->add_work([=](void *) {
        Ref<Image> _height_map = height_map;
        std::vector<f32> height_field;
        u32 sample_cnt = std::max(width, depth);
        height_field.resize(sample_cnt * sample_cnt);
        for (int i = 0; i < sample_cnt; i++) {
            for (int j = 0; j < sample_cnt; j++) {
                if (i >= depth || j >= width) {
                    height_field[i * sample_cnt + j] = FLT_MIN;
                } else {
                    // get height from y value
                    f32 height =
                        (f32)_height_map->get_data()[(i * width + j) * 4 + 1];

                    height_field[i * sample_cnt + j] = height;
                }
            }
        }
        PhysicHeightmapShape shape(height_field.data(), sample_cnt,
                                   Vec3{left, -16, top}, Vec3{1, 0.25f, 1});

        PhysicEngine::get_instance()->create_body(
            this->body, shape, PhysicBodyType::STATIC, Vec3{0, 0, 0});
    });

    RenderResource vertices_rc;
    vertices_rc.alloc_vertex(sizeof(TerrainVertex), vertices.size(),
                             vertices.data());
    this->vertices.bind_vertices(sizeof(TerrainVertex), vertices.size(),
                                 vertices_rc);
    Ref<Texture> height_map_tex = height_map->create_texture();
    terrain_mat.create(height_map_tex);
}

Terrain::~Terrain() {}
}  // namespace Seed