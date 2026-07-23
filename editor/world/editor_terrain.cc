#include "editor_terrain.h"

#include <algorithm>
#include <cfloat>

#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/default_storage.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/system.h"
#include "editor/editor_storage.h"

namespace Seed {

void EditorTerrain::build_mesh() {
    constexpr u32 kPatchCount = 4;
    constexpr u32 kVertexRowCount = kPatchCount + 1;
    constexpr u32 kVerticesPerPatch = 4;
    const f32 vertex_spacing = (f32)CHUNK_SIZE / (f32)kPatchCount;

    std::vector<TerrainVertex> vertices;
    vertices.reserve(kVertexRowCount * kVertexRowCount);
    for (u32 row = 0; row < kVertexRowCount; row++) {
        for (u32 column = 0; column < kVertexRowCount; column++) {
            vertices.push_back(TerrainVertex{
                Vec2{vertex_spacing * (f32)row - CHUNK_SIZE / 2.0f,
                     vertex_spacing * (f32)column - CHUNK_SIZE / 2.0f}});
        }
    }

    std::vector<u32> indices;
    indices.reserve(kPatchCount * kPatchCount * kVerticesPerPatch);
    for (u32 row = 0; row < kPatchCount; row++) {
        for (u32 column = 0; column < kPatchCount; column++) {
            const u32 offset = column + row * kVertexRowCount;
            indices.push_back(offset);
            indices.push_back(offset + 1);
            indices.push_back(offset + kVertexRowCount);
            indices.push_back(offset + kVertexRowCount + 1);
        }
    }

    mesh.create(&System::gDefaultStorage->terrain_desc, vertices, indices,
                AABB{.center = Vec3{0, 0, 0},
                     .ext = Vec3{CHUNK_SIZE / 2, 0, CHUNK_SIZE / 2}});
    mesh->set_type(RenderPrimitiveType::PATCHES);
}

EditorTerrain::EditorTerrain() {
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

    fallback_texture.create(PixelFormat::RGBA, TERRAIN_TEXTURE_SIZE,
                            TERRAIN_TEXTURE_SIZE);
    fallback_texture->fill(Color{128, 128, 128, 255}, TERRAIN_TEXTURE_SIZE,
                           TERRAIN_TEXTURE_SIZE);
    fallback_normal.create(PixelFormat::RGBA, TERRAIN_TEXTURE_SIZE,
                           TERRAIN_TEXTURE_SIZE);
    fallback_normal->fill(Color{128, 128, 255, 255}, TERRAIN_TEXTURE_SIZE,
                          TERRAIN_TEXTURE_SIZE);
    reset_texture_palette();

    material.create(System::gEditorStorage->editor_terrain_shader, heightmaps,
                    controlmaps, textures, texture_normals);
    instances.create();

    build_mesh();
    mesh->set_material(ref_cast<Material>(material));
}

void EditorTerrain::upload_fallback_layer(u32 layer) {
    if (layer >= TERRAIN_TEXTURE_LAYERS || fallback_texture.is_null() ||
        fallback_normal.is_null()) {
        return;
    }

    textures->update_layer(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE, layer,
                           fallback_texture->get_data());
    texture_normals->update_layer(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE,
                                  layer, fallback_normal->get_data());
}

bool EditorTerrain::update_texture_layer(u32 layer,
                                         RHI::UpdateBufferInfo info) {
    if (layer >= TERRAIN_TEXTURE_LAYERS) return false;
    if (info.data == nullptr) {
        textures->update_layer(TERRAIN_TEXTURE_SIZE, TERRAIN_TEXTURE_SIZE,
                               layer, fallback_texture->get_data());
        return false;
    }

    textures->update_layer(layer, info);
    return true;
}

bool EditorTerrain::update_normal_layer(u32 layer, RHI::UpdateBufferInfo info) {
    if (layer >= TERRAIN_TEXTURE_LAYERS) return false;
    if (info.data == nullptr) {
        texture_normals->update_layer(TERRAIN_TEXTURE_SIZE,
                                      TERRAIN_TEXTURE_SIZE, layer,
                                      fallback_normal->get_data());
        return false;
    }

    texture_normals->update_layer(layer, info);
    return true;
}

void EditorTerrain::reset_texture_palette() {
    for (u32 layer = 0; layer < TERRAIN_TEXTURE_LAYERS; layer++) {
        upload_fallback_layer(layer);
    }
}

void EditorTerrain::add_chunk(i32 x, i32 y, Ref<Image> heightmap,
                              Ref<Image> controlmap) {
    const u32 chunk_index = tile_map.add_tile(x, y, heightmap, controlmap);

    if (!heightmap.is_null()) {
        heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                                 heightmap->get_data());
    }
    if (!controlmap.is_null()) {
        controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                                  controlmap->get_data());
    }

    f32 max_height = 0.0f;
    f32 min_height = 0.0f;
    if (!heightmap.is_null()) {
        max_height = -FLT_MAX;
        min_height = FLT_MAX;
        for (i32 pixel_x = 0; pixel_x < CHUNK_SIZE; pixel_x++) {
            for (i32 pixel_y = 0; pixel_y < CHUNK_SIZE; pixel_y++) {
                const f32 height =
                    (f32)heightmap->pixel(pixel_x + HEIGHTMAP_INNER_FIRST,
                                          pixel_y + HEIGHTMAP_INNER_FIRST)[1] *
                        HEIGHT_SCALE +
                    HEIGHT_OFFSET;
                max_height = std::max(max_height, height);
                min_height = std::min(min_height, height);
            }
        }
    }

    instances->insert_terrain_data(TerrainInstance{
        .pos = Vec2{(f32)(x * CHUNK_SIZE), (f32)(y * CHUNK_SIZE)},
        .heightmap_index = chunk_index,
        .max_height = max_height,
        .min_height = min_height});
}

void EditorTerrain::clear_chunks() {
    tile_map.clear();
    instances->clear();
}

bool EditorTerrain::chunk_exists_at(i32 x, i32 y) const {
    return tile_map.tile_exists_at(x, y);
}

bool EditorTerrain::read_height(i32 world_x, i32 world_y, u8 &height) {
    return tile_map.read_height(world_x, world_y, height);
}

bool EditorTerrain::apply_brush(i32 x, i32 y, TerrainBrush brush,
                                const TerrainBrushSetting &setting) {
    TerrainEdit edit = apply_terrain_brush(tile_map, x, y, brush, setting);
    if (!edit.has_changes()) return false;

    commit_edit(edit);
    return true;
}

void EditorTerrain::commit_edit(TerrainEdit &edit) {
    tile_map.commit(edit);
    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);
}

void EditorTerrain::connect_chunk(u32 chunk_index) {
    TerrainEdit edit = tile_map.connect_tile(chunk_index);
    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);
}

void EditorTerrain::sync_loaded_tile_seams() {
    TerrainEdit edit = tile_map.synchronize_loaded_tiles();
    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);
}

void EditorTerrain::upload_heightmaps(const std::set<u32> &touched_chunks) {
    for (u32 chunk_index : touched_chunks) {
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (tile == nullptr || tile->heightmap.is_null()) continue;
        heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                                 tile->heightmap->get_data());
    }
}

void EditorTerrain::upload_controlmaps(const std::set<u32> &touched_chunks) {
    for (u32 chunk_index : touched_chunks) {
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (tile == nullptr || tile->controlmap.is_null()) continue;
        controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                                  tile->controlmap->get_data());
    }
}

void EditorTerrain::clear_dirty_maps() { tile_map.clear_dirty_maps(); }

void EditorTerrain::save_dirty_maps(const std::vector<ChunkSetting> &chunks) {
    if (!tile_map.has_dirty_maps() || System::gResourceLoader == nullptr) {
        return;
    }

    for (u32 chunk_index : tile_map.get_dirty_heightmaps()) {
        if (chunk_index >= chunks.size()) continue;

        ResourceEntry *entry =
            System::gResourceEntries->get_entry(chunks[chunk_index].height_map);
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (entry == nullptr || tile == nullptr || tile->heightmap.is_null()) {
            continue;
        }
        tile->heightmap->save_disk(entry->real_path());
    }

    for (u32 chunk_index : tile_map.get_dirty_controlmaps()) {
        if (chunk_index >= chunks.size()) continue;

        ResourceEntry *entry = System::gResourceEntries->get_entry(
            chunks[chunk_index].control_map);
        EditorTile *tile = tile_map.get_tile(chunk_index);
        if (entry == nullptr || tile == nullptr || tile->controlmap.is_null()) {
            continue;
        }
        tile->controlmap->save_disk(entry->real_path());
    }

    clear_dirty_maps();
}

}  // namespace Seed
