#include "editor_terrain.h"
#include <stb_image.h>
#include <fmt/format.h>
#include "core/ref.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/image.h"
#include "core/resource/resource_loader.h"
#include "core/resource/texture.h"
#include "editor/editor_storage.h"

namespace Seed {

#define CHUNK_SIZE (256)
#define HEIGHTMAP_INNER_SIZE (CHUNK_SIZE + 1)
#define HEIGHTMAP_BORDER (1)
#define HEIGHTMAP_SIZE (HEIGHTMAP_INNER_SIZE + HEIGHTMAP_BORDER * 2)
#define HEIGHTMAP_INNER_FIRST (HEIGHTMAP_BORDER)
#define HEIGHT_OFFSET (-128)
#define HEIGHT_SCALE (1)

EditorTerrainMaterial::EditorTerrainMaterial(Ref<TextureArray> heightmaps,
                                             Ref<TextureArray> controlmaps,
                                             Ref<TextureArray> textures,
                                             Ref<TextureArray> texture_normals)
    : Material(ES::get_instance()->editor_terrain_shader) {
    this->set_texture("height_map", ref_cast<Texture>(heightmaps));
    this->set_texture("control_map", ref_cast<Texture>(controlmaps));
    this->set_texture("textures", ref_cast<Texture>(textures));
    this->set_texture("texture_normals", ref_cast<Texture>(texture_normals));
    this->raster_state = {.cull_mode = Cullmode::FRONT,
                          .patch_control_points = 4};
}

Ref<Texture> EditorTerrainMaterial::get_height_map() {
    return this->get_texture("height_map");
}

void EditorTerrain::build_mesh() {
    u32 chunk_cnt = 4;
    u32 vertex_row_cnt = chunk_cnt + 1;
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
    heightmaps.create(TextureType::TEXTURE_2D_ARRAY, HEIGHTMAP_SIZE,
                      HEIGHTMAP_SIZE, 256, PixelFormat::RG, SamplerProperty{});
    controlmaps.create(TextureType::TEXTURE_2D_ARRAY, HEIGHTMAP_SIZE,
                       HEIGHTMAP_SIZE, 256, PixelFormat::RGBA,
                       SamplerProperty{.min_filter = SamplerFilter::NEAREST,
                                       .mag_filter = SamplerFilter::NEAREST});
    textures.create(TextureType::TEXTURE_2D_ARRAY, EDITOR_TERRAIN_TEXTURE_SIZE,
                    EDITOR_TERRAIN_TEXTURE_SIZE, EDITOR_TERRAIN_TEXTURE_LAYERS,
                    PixelFormat::RGBA,
                    SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                    .wrap_v = SamplerWrap::REPEAT});
    texture_normals.create(TextureType::TEXTURE_2D_ARRAY,
                           EDITOR_TERRAIN_TEXTURE_SIZE,
                           EDITOR_TERRAIN_TEXTURE_SIZE,
                           EDITOR_TERRAIN_TEXTURE_LAYERS, PixelFormat::RGBA,
                           SamplerProperty{.wrap_u = SamplerWrap::REPEAT,
                                           .wrap_v = SamplerWrap::REPEAT});
    material.create(heightmaps, controlmaps, textures, texture_normals);

    this->instances.create();

    build_mesh();
    this->mesh->set_material(ref_cast<Material>(material));
}

void EditorTerrain::add_chunk(i32 x, i32 y, Ref<Image> height_map,
                              Ref<Image> control_map) {
    heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, last_heightmap,
                             height_map->get_data());
    controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, last_heightmap,
                              control_map->get_data());

    f32 max_height = -FLT_MAX;
    f32 min_height = FLT_MAX;

    for (i32 i = 0; i < CHUNK_SIZE; i++) {
        for (i32 j = 0; j < CHUNK_SIZE; j++) {
            // get height from y value
            f32 height = (f32)height_map->pixel(i + HEIGHTMAP_INNER_FIRST,
                                                j + HEIGHTMAP_INNER_FIRST)[1] *
                             HEIGHT_SCALE +
                         HEIGHT_OFFSET;
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

void EditorTerrain::clear_chunks() {
    last_heightmap = 0;
    instances->clear();
}

void EditorTerrain::update_chunk_heightmap(u32 chunk_index,
                                           Ref<Image> height_map) {
    heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                             height_map->get_data());
}

void EditorTerrain::update_chunk_controlmap(u32 chunk_index,
                                            Ref<Image> control_map) {
    if (control_map.is_null()) return;
    controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                              control_map->get_data());
}

static bool update_terrain_texture_array_layer(Ref<TextureArray> texture_array,
                                               u32 layer, UUID texture,
                                               bool force_rgba,
                                               u32 expected_pixel_size) {
    if (texture_array.is_null() || texture.is_null() ||
        layer >= EDITOR_TERRAIN_TEXTURE_LAYERS) {
        return false;
    }

    RHI::UpdateBufferInfo info =
        ResourceLoader::get_instance()->load_image_to_upload(texture,
                                                             force_rgba);
    if (info.data == nullptr) return false;

    if (info.image.w != EDITOR_TERRAIN_TEXTURE_SIZE ||
        info.image.h != EDITOR_TERRAIN_TEXTURE_SIZE ||
        info.image.pixel_size != expected_pixel_size) {
        stbi_image_free(info.data);
        return false;
    }

    RHI::update_from_heap(texture_array->get_handle(), layer, 0, 0, info);
    return true;
}

bool EditorTerrain::update_texture_layer(u32 layer, UUID texture) {
    return update_terrain_texture_array_layer(textures, layer, texture, true,
                                              4);
}

bool EditorTerrain::update_normal_layer(u32 layer, UUID texture) {
    return update_terrain_texture_array_layer(texture_normals, layer, texture,
                                              false, 3);
}

EditorTerrain::~EditorTerrain() {}
}  // namespace Seed
