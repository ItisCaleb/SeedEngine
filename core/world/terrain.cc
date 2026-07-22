#include "terrain.h"
#include "core/math/vec2.h"
#include "core/math/vec4.h"
#include "core/resource/default_storage.h"
#include "core/physic/physic_engine.h"
#include "core/rendering/mesh_storage.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/texture.h"
#include "core/system.h"
#include "core/types.h"
#include <math.h>
#include <cfloat>

namespace Seed {

TerrainMaterial::TerrainMaterial(Ref<Shader> shader,
                                 Ref<TextureArray> heightmaps,
                                 Ref<TextureArray> controlmaps,
                                 Ref<TextureArray> textures,
                                 Ref<TextureArray> texture_normals)
    : Material(shader) {
    this->set_texture("height_map", ref_cast<Texture>(heightmaps));
    this->set_texture("control_map", ref_cast<Texture>(controlmaps));
    this->set_texture("textures", ref_cast<Texture>(textures));
    this->set_texture("texture_normals", ref_cast<Texture>(texture_normals));
    this->set_texture("noise_texture", System::gDefaultStorage->noise_texture);
    this->textures = textures;
    this->texture_normals = texture_normals;
    this->raster_state = {.cull_mode = Cullmode::BACK,
                          .patch_control_points = 4};
    this->depth_state = {.depth_mode = DepthMode::OPAQUE};
}

TerrainMaterial::TerrainMaterial(Ref<TextureArray> heightmaps,
                                 Ref<TextureArray> controlmaps,
                                 Ref<TextureArray> textures,
                                 Ref<TextureArray> texture_normals)
    : TerrainMaterial(System::gDefaultStorage->terrain_shader, heightmaps,
                      controlmaps, textures, texture_normals) {}

void TerrainInstanceData::insert_terrain_data(const TerrainInstance &instance) {
    this->instances.push_back(instance);
}

void TerrainInstanceData::upload() {
    u32 stride = sizeof(Vec4);
    RHI::UpdateBufferInfo instance_info =
        RHI::alloc_heap(stride * this->instances.size());
    void *instance_data = instance_info.data;
    u32 i = 0;
    for (TerrainInstance &instance : this->instances) {
        memcpy((void *)((u64)instance_data + stride * i), &instance, 12);
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
          System::gRenderEngine->get_instance_pool(TERRAIN_POOL_NAME)) {}

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

    this->mesh.create(&System::gDefaultStorage->terrain_desc, vertices, indices,
                      AABB{.center = Vec3{0, 0, 0},
                           .ext = Vec3{CHUNK_SIZE / 2, 0, CHUNK_SIZE / 2}});
    this->mesh->set_type(RenderPrimitiveType::PATCHES);
}

Terrain::Terrain() {
    heightmaps.create(TextureType::TEXTURE_2D_ARRAY, HEIGHTMAP_SIZE,
                      HEIGHTMAP_SIZE, 256, PixelFormat::RG, SamplerProperty{});
    controlmaps.create(TextureType::TEXTURE_2D_ARRAY, HEIGHTMAP_SIZE,
                       HEIGHTMAP_SIZE, 256, PixelFormat::RGBA,
                       SamplerProperty{.min_filter = SamplerFilter::NEAREST,
                                       .mag_filter = SamplerFilter::NEAREST});
    textures.create(TextureType::TEXTURE_2D_ARRAY, TERRAIN_TEXTURE_SIZE,
                    TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_LAYERS,
                    PixelFormat::RGBA,
                    SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                    .wrap_v = SamplerWrap::REPEAT});
    texture_normals.create(TextureType::TEXTURE_2D_ARRAY, TERRAIN_TEXTURE_SIZE,
                           TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_LAYERS,
                           PixelFormat::RGBA,
                           SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                           .wrap_v = SamplerWrap::REPEAT});
    material.create(heightmaps, controlmaps, textures, texture_normals);
    this->instances.create();

    build_mesh();
    this->mesh->set_material(ref_cast<Material>(material));
    System::gRenderEngine->get_mesh_storage()->add_mesh(
        this->mesh, ref_cast<InstanceData>(this->instances));
}

void Terrain::add_chunk(i32 x, i32 y, Ref<Image> height_map,
                        Ref<Image> control_map) {
    heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, last_heightmap,
                             height_map->get_data());
    controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, last_heightmap,
                              control_map->get_data());

    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;

    std::vector<f32> height_field;
    height_field.resize(CHUNK_SIZE * CHUNK_SIZE);

    for (i32 i = 0; i < CHUNK_SIZE; i++) {
        for (i32 j = 0; j < CHUNK_SIZE; j++) {
            // get height from y value
            f32 height = (f32)height_map->pixel(i + HEIGHTMAP_INNER_FIRST,
                                                j + HEIGHTMAP_INNER_FIRST)[1] *
                             HEIGHT_SCALE +
                         HEIGHT_OFFSET;
            max_height = std::max(max_height, height);
            min_height = std::min(min_height, height);
            height_field[i * CHUNK_SIZE + j] = height;
        }
    }
    f32 wx = (f32)(x * CHUNK_SIZE);
    f32 wy = (f32)(y * CHUNK_SIZE);
    instances->insert_terrain_data(
        TerrainInstance{.pos = Vec2{wx, wy},
                        .heightmap_index = last_heightmap,
                        .max_height = max_height,
                        .min_height = min_height});

    PhysicBody &body = this->bodies.emplace_back();
    PhysicHeightmapShape shape(
        height_field.data(), CHUNK_SIZE,
        Vec3{-(i32)CHUNK_SIZE / 2, 0, -(i32)CHUNK_SIZE / 2});

    System::gPhysicEngine->create_body(body, shape, PhysicBodyType::STATIC,
                                       Vec3{wx, 0, wy});
    this->instances->upload();

    last_heightmap++;
}

Terrain::~Terrain() {
    System::gRenderEngine->get_mesh_storage()->remove_mesh(
        this->mesh, ref_cast<InstanceData>(this->instances));
}
}  // namespace Seed
