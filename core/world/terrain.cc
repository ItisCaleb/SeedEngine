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

namespace {

struct HeightRange {
        f32 min = FLT_MAX;
        f32 max = -FLT_MAX;
};

HeightRange read_heightmap(Ref<Image> heightmap,
                           std::vector<f32> *height_field = nullptr) {
    HeightRange range;
    if (height_field != nullptr) {
        height_field->resize(CHUNK_SIZE * CHUNK_SIZE);
    }

    for (i32 x = 0; x < CHUNK_SIZE; x++) {
        for (i32 y = 0; y < CHUNK_SIZE; y++) {
            const f32 height =
                (f32)heightmap
                    ->pixel(x + HEIGHTMAP_INNER_FIRST,
                            y + HEIGHTMAP_INNER_FIRST)[1] *
                    HEIGHT_SCALE +
                HEIGHT_OFFSET;
            range.max = std::max(range.max, height);
            range.min = std::min(range.min, height);
            if (height_field != nullptr) {
                (*height_field)[x * CHUNK_SIZE + y] = height;
            }
        }
    }
    return range;
}

}  // namespace

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

void TerrainInstanceBatch::insert_terrain_data(
    const TerrainInstance &instance) {
    this->instances.push_back(instance);
    mark_dirty();
}

void TerrainInstanceBatch::update_height_range(u32 index, f32 min_height,
                                               f32 max_height) {
    if (index >= instances.size()) return;

    instances[index].min_height = min_height;
    instances[index].max_height = max_height;
    mark_dirty();
}

void TerrainInstanceBatch::prepare_uploads(
    std::vector<RHI::UpdateBufferInfo> &uploads) {
    u32 stride = sizeof(Vec4);
    RHI::UpdateBufferInfo instance_info =
        RHI::alloc_heap(stride * this->instances.size());
    void *instance_data = instance_info.data;
    u32 i = 0;
    for (TerrainInstance &instance : this->instances) {
        memcpy((void *)((u64)instance_data + stride * i), &instance, 12);
        i++;
    }
    uploads.push_back(instance_info);
}
AABB TerrainInstanceBatch::translate_bounding_box(const AABB &bounding_box,
                                                  u32 i) {
    TerrainInstance &instance = instances[i];
    AABB aabb = bounding_box;
    f32 mid_height = (instance.max_height + instance.min_height) / 2.0f;
    aabb.center.x = instance.pos.x;
    aabb.center.z = instance.pos.y;
    aabb.center.y = mid_height;
    aabb.ext.y = instance.max_height - mid_height;
    return aabb;
}

TerrainInstanceBatch::TerrainInstanceBatch() {}

void Terrain::build_mesh() {
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
                      HEIGHTMAP_SIZE, TERRAIN_CHUNK_LAYERS, PixelFormat::RG,
                      SamplerProperty{});
    controlmaps.create(TextureType::TEXTURE_2D_ARRAY, HEIGHTMAP_SIZE,
                       HEIGHTMAP_SIZE, TERRAIN_CHUNK_LAYERS, PixelFormat::RGBA,
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

    fallback_texture.create(PixelFormat::RGBA, TERRAIN_TEXTURE_SIZE,
                            TERRAIN_TEXTURE_SIZE);
    fallback_texture->fill(Color{128, 128, 128, 255}, TERRAIN_TEXTURE_SIZE,
                           TERRAIN_TEXTURE_SIZE);
    fallback_normal.create(PixelFormat::RGBA, TERRAIN_TEXTURE_SIZE,
                           TERRAIN_TEXTURE_SIZE);
    fallback_normal->fill(Color{128, 128, 255, 255}, TERRAIN_TEXTURE_SIZE,
                          TERRAIN_TEXTURE_SIZE);
    reset_texture_palette();
    Ref<TerrainMaterial> terrain_material;
    terrain_material.create(heightmaps, controlmaps, textures,
                        texture_normals);
    this->material = ref_cast<Material>(terrain_material);
    this->instances.create();

    build_mesh();
    this->mesh->set_material(material);
    System::gRenderEngine->get_mesh_storage()->add_mesh(
        this->mesh, ref_cast<InstanceBatch>(this->instances));
}

void Terrain::upload_fallback_layer(u32 layer) {
    if (layer >= TERRAIN_TEXTURE_LAYERS) return;

    textures->update_layer(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE, layer,
                           fallback_texture->get_data());
    texture_normals->update_layer(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE,
                                  layer, fallback_normal->get_data());
}

void Terrain::reset_texture_palette() {
    for (u32 layer = 0; layer < TERRAIN_TEXTURE_LAYERS; layer++) {
        upload_fallback_layer(layer);
    }
}

bool Terrain::update_texture_layer(u32 layer, RHI::UpdateBufferInfo info) {
    if (layer >= TERRAIN_TEXTURE_LAYERS) return false;

    if (info.data == nullptr) {
        textures->update_layer(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE,
                               layer, fallback_texture->get_data());
    } else {
        textures->update_layer(layer, info);
    }
    return true;
}

bool Terrain::update_normal_layer(u32 layer, RHI::UpdateBufferInfo info) {
    if (layer >= TERRAIN_TEXTURE_LAYERS) return false;

    if (info.data == nullptr) {
        texture_normals->update_layer(TERRAIN_TEXTURE_SIZE,
                                      TERRAIN_TEXTURE_SIZE, layer,
                                      fallback_normal->get_data());
    } else {
        texture_normals->update_layer(layer, info);
    }
    return true;
}

void Terrain::update_heightmap_layer(u32 layer, Ref<Image> heightmap) {
    if (layer >= TERRAIN_CHUNK_LAYERS || heightmap.is_null()) return;
    heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, layer,
                             heightmap->get_data());
    if (layer < instances->size()) {
        const HeightRange range = read_heightmap(heightmap);
        instances->update_height_range(layer, range.min, range.max);
    }
}

void Terrain::update_controlmap_layer(u32 layer, Ref<Image> controlmap) {
    if (layer >= TERRAIN_CHUNK_LAYERS || controlmap.is_null()) return;
    controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, layer,
                              controlmap->get_data());
}

void Terrain::refresh_chunk_collision(u32 layer, Ref<Image> heightmap) {
    if (System::gPhysicEngine == nullptr || heightmap.is_null() ||
        layer >= bodies.size() || layer >= chunk_positions.size()) {
        return;
    }

    std::vector<f32> height_field;
    read_heightmap(heightmap, &height_field);

    PhysicBody &body = bodies[layer];
    System::gPhysicEngine->delete_body(body);

    PhysicHeightmapShape shape(
        height_field.data(), CHUNK_SIZE,
        Vec3{-(i32)CHUNK_SIZE / 2, 0, -(i32)CHUNK_SIZE / 2});
    const Vec2 &position = chunk_positions[layer];
    System::gPhysicEngine->create_body(
        body, shape, PhysicBodyType::STATIC,
        Vec3{position.x, 0.0f, position.y});
}

void Terrain::add_chunk(i32 x, i32 y, Ref<Image> height_map,
                        Ref<Image> control_map) {
    if (last_heightmap >= TERRAIN_CHUNK_LAYERS || height_map.is_null() ||
        control_map.is_null()) {
        return;
    }

    update_heightmap_layer(last_heightmap, height_map);
    update_controlmap_layer(last_heightmap, control_map);

    std::vector<f32> height_field;
    const HeightRange range = read_heightmap(height_map, &height_field);
    f32 wx = (f32)(x * CHUNK_SIZE);
    f32 wy = (f32)(y * CHUNK_SIZE);
    instances->insert_terrain_data(
        TerrainInstance{.pos = Vec2{wx, wy},
                        .heightmap_index = last_heightmap,
                        .max_height = range.max,
                        .min_height = range.min});
    chunk_positions.push_back(Vec2{wx, wy});

    PhysicBody &body = this->bodies.emplace_back();
    if (System::gPhysicEngine != nullptr) {
        PhysicHeightmapShape shape(
            height_field.data(), CHUNK_SIZE,
            Vec3{-(i32)CHUNK_SIZE / 2, 0, -(i32)CHUNK_SIZE / 2});

        System::gPhysicEngine->create_body(body, shape, PhysicBodyType::STATIC,
                                           Vec3{wx, 0, wy});
    }
    last_heightmap++;
}

void Terrain::clear_chunks() {
    if (System::gPhysicEngine != nullptr) {
        for (PhysicBody &body : bodies) {
            System::gPhysicEngine->delete_body(body);
        }
    }
    bodies.clear();
    chunk_positions.clear();
    instances->clear();
    last_heightmap = 0;
}

Terrain::~Terrain() {
    clear_chunks();
    System::gRenderEngine->get_mesh_storage()->remove_mesh(
        this->mesh, ref_cast<InstanceBatch>(this->instances));
}
}  // namespace Seed
