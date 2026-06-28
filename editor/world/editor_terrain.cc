#include "editor_terrain.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include "core/ref.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/image.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/resource/texture.h"
#include "editor/editor_storage.h"

namespace Seed {

namespace {

constexpr i32 kTileSize = HEIGHTMAP_INNER_SIZE;
constexpr i32 kTileFirst = HEIGHTMAP_BORDER;
constexpr i32 kTileLast = kTileFirst + kTileSize - 1;
constexpr i32 kTilePadFirst = 0;
constexpr i32 kTilePadLast = HEIGHTMAP_SIZE - 1;

}  // namespace

void EditorTile::clamp_border(Ref<Image> image) {
    if (image.is_null()) return;

    image->copy_column(image, kTileFirst, kTileFirst, kTilePadFirst,
                       kTileFirst, kTileSize);
    image->copy_column(image, kTileLast, kTileFirst, kTilePadLast, kTileFirst,
                       kTileSize);
    image->copy_row(image, kTilePadFirst, kTileFirst, kTilePadFirst,
                    kTilePadFirst, HEIGHTMAP_SIZE);
    image->copy_row(image, kTilePadFirst, kTileLast, kTilePadFirst,
                    kTilePadLast, HEIGHTMAP_SIZE);
}

void EditorTile::clamp_border() {
    clamp_border(heightmap);
    clamp_border(controlmap);
}

Ref<Image> EditorTile::get_image(TileImage image_type) const {
    switch (image_type) {
        case TileImage::Heightmap:
            return heightmap;
        case TileImage::Controlmap:
            return controlmap;
    }
    return Ref<Image>();
}

void EditorTile::clamp_border(TileImage image_type) {
    clamp_border(get_image(image_type));
}

bool EditorTile::build_edge_from_image(Ref<Image> image, Ref<Image> source,
                                       TileDirection direction) {
    if (image.is_null() || source.is_null()) return false;

    switch (direction) {
        case TileDirection::LEFT:
            return source->copy_column(image, kTileLast, kTileFirst,
                                       kTileFirst, kTileFirst, kTileSize);
        case TileDirection::RIGHT:
            return source->copy_column(image, kTileFirst, kTileFirst,
                                       kTileLast, kTileFirst, kTileSize);
        case TileDirection::BOTTOM:
            return source->copy_row(image, kTileFirst, kTileLast, kTileFirst,
                                    kTileFirst, kTileSize);
        case TileDirection::UP:
            return source->copy_row(image, kTileFirst, kTileFirst, kTileFirst,
                                    kTileLast, kTileSize);
    }
    return false;
}

bool EditorTile::build_edge_from_tile(EditorTile *tile,
                                      TileDirection direction) {
    if (tile == nullptr) return false;

    bool copied = false;
    if (build_edge_from_image(heightmap, tile->heightmap, direction)) {
        copied = true;
    }
    if (build_edge_from_image(controlmap, tile->controlmap, direction)) {
        copied = true;
    }
    return copied;
}

bool EditorTile::build_edge_from_tile(EditorTile *tile,
                                      TileDirection direction,
                                      TileImage image_type) {
    if (tile == nullptr) return false;
    return build_edge_from_image(get_image(image_type),
                                 tile->get_image(image_type), direction);
}

bool EditorTile::build_border_from_image(Ref<Image> image, Ref<Image> source,
                                         TileDirection direction) {
    if (image.is_null() || source.is_null()) return false;

    switch (direction) {
        case TileDirection::LEFT:
            return source->copy_column(image, kTileLast - 1, kTileFirst,
                                       kTilePadFirst, kTileFirst, kTileSize);
        case TileDirection::RIGHT:
            return source->copy_column(image, kTileFirst + 1, kTileFirst,
                                       kTilePadLast, kTileFirst, kTileSize);
        case TileDirection::BOTTOM:
            return source->copy_row(image, kTileFirst, kTileLast - 1,
                                    kTileFirst, kTilePadFirst, kTileSize);
        case TileDirection::UP:
            return source->copy_row(image, kTileFirst, kTileFirst + 1,
                                    kTileFirst, kTilePadLast, kTileSize);
    }
    return false;
}

bool EditorTile::build_border_from_tile(EditorTile *tile,
                                        TileDirection direction) {
    if (tile == nullptr) return false;

    bool copied = false;
    if (build_border_from_image(heightmap, tile->heightmap, direction)) {
        copied = true;
    }
    if (build_border_from_image(controlmap, tile->controlmap, direction)) {
        copied = true;
    }
    return copied;
}

bool EditorTile::build_border_from_tile(EditorTile *tile,
                                        TileDirection direction,
                                        TileImage image_type) {
    if (tile == nullptr) return false;
    return build_border_from_image(get_image(image_type),
                                   tile->get_image(image_type), direction);
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
                offset * i - CHUNK_SIZE / 2, offset * j - CHUNK_SIZE / 2}});
        }
    }

    std::vector<u32> indices;
    for (i32 i = 0; i < chunk_cnt; i++) {
        for (i32 j = 0; j < chunk_cnt; j++) {
            i32 chunk_offset = j * step + i * step * vertex_row_cnt;
            indices.push_back(chunk_offset);
            indices.push_back(chunk_offset + step);
            indices.push_back(chunk_offset + vertex_row_cnt * step);
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
    material.create(ES::get_instance()->editor_terrain_shader, heightmaps,
                    controlmaps, textures, texture_normals);
    this->instances.create();

    build_mesh();
    this->mesh->set_material(ref_cast<Material>(material));
}

void EditorTerrain::add_chunk(i32 x, i32 y, Ref<Image> height_map,
                              Ref<Image> control_map) {
    u32 chunk_index = last_heightmap;

    if (!height_map.is_null()) {
        heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                                 height_map->get_data());
    }
    if (!control_map.is_null()) {
        controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                                  control_map->get_data());
    }

    f32 max_height = 0.0f;
    f32 min_height = 0.0f;
    if (!height_map.is_null()) {
        max_height = -FLT_MAX;
        min_height = FLT_MAX;
        for (i32 i = 0; i < CHUNK_SIZE; i++) {
            for (i32 j = 0; j < CHUNK_SIZE; j++) {
                f32 height =
                    (f32)height_map->pixel(i + HEIGHTMAP_INNER_FIRST,
                                           j + HEIGHTMAP_INNER_FIRST)[1] *
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

    tiles[chunk_index] = EditorTile{.x = x,
                                    .y = y,
                                    .heightmap = height_map,
                                    .controlmap = control_map};
    pos_to_index[std::pair<i32, i32>(x, y)] = chunk_index;
    last_heightmap++;
}

void EditorTerrain::clear_chunks() {
    last_heightmap = 0;
    pos_to_index.clear();
    tiles.clear();
    dirty_heightmaps.clear();
    dirty_controlmaps.clear();
    instances->clear();
}

void EditorTerrain::update_chunk_heightmap(u32 chunk_index,
                                           Ref<Image> height_map) {
    if (height_map.is_null()) return;
    heightmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                             height_map->get_data());
}

void EditorTerrain::update_chunk_controlmap(u32 chunk_index,
                                            Ref<Image> control_map) {
    if (control_map.is_null()) return;
    controlmaps->update_layer(HEIGHTMAP_SIZE, HEIGHTMAP_SIZE, chunk_index,
                              control_map->get_data());
}

i32 EditorTerrain::find_chunk_index_at(i32 x, i32 y) const {
    std::map<std::pair<i32, i32>, u32>::const_iterator iter =
        pos_to_index.find(std::pair<i32, i32>(x, y));
    if (iter == pos_to_index.end()) return -1;
    return (i32)iter->second;
}

bool EditorTerrain::chunk_exists_at(i32 x, i32 y) const {
    return find_chunk_index_at(x, y) >= 0;
}

EditorTile *EditorTerrain::get_tile_at(i32 x, i32 y) {
    i32 index = find_chunk_index_at(x, y);
    if (index < 0) return nullptr;
    return get_tile((u32)index);
}

EditorTile *EditorTerrain::get_tile(u32 index) {
    std::map<u32, EditorTile>::iterator iter = tiles.find(index);
    if (iter == tiles.end()) return nullptr;
    return &iter->second;
}

i32 EditorTerrain::find_chunk_index(i32 world_x, i32 world_y) const {
    i32 best_idx = -1;
    i32 best_score = -1;
    for (const auto &entry : tiles) {
        const EditorTile &tile = entry.second;
        i32 chunk_origin_x = tile.x * CHUNK_SIZE - CHUNK_SIZE / 2;
        i32 chunk_origin_y = tile.y * CHUNK_SIZE - CHUNK_SIZE / 2;
        i32 local_x = world_x - chunk_origin_x;
        i32 local_y = world_y - chunk_origin_y;
        if (local_x < 0 || local_y < 0 || local_x > CHUNK_SIZE ||
            local_y > CHUNK_SIZE) {
            continue;
        }

        i32 score = 0;
        if (local_x == CHUNK_SIZE) score++;
        if (local_y == CHUNK_SIZE) score++;
        if (score > best_score) {
            best_idx = (i32)entry.first;
            best_score = score;
        }
    }
    return best_idx;
}

bool EditorTerrain::world_to_heightmap_pixel(i32 world_x, i32 world_y,
                                             u32 &chunk_idx, u32 &pixel_x,
                                             u32 &pixel_y) const {
    i32 idx = find_chunk_index(world_x, world_y);
    if (idx < 0) return false;

    std::map<u32, EditorTile>::const_iterator iter = tiles.find((u32)idx);
    if (iter == tiles.end()) return false;

    const EditorTile &tile = iter->second;
    i32 chunk_origin_x = tile.x * CHUNK_SIZE - CHUNK_SIZE / 2;
    i32 chunk_origin_y = tile.y * CHUNK_SIZE - CHUNK_SIZE / 2;
    i32 local_x = world_x - chunk_origin_x;
    i32 local_y = world_y - chunk_origin_y;
    if (local_x < 0 || local_y < 0 || local_x > CHUNK_SIZE ||
        local_y > CHUNK_SIZE) {
        return false;
    }

    chunk_idx = (u32)idx;
    pixel_x = (u32)local_x + HEIGHTMAP_BORDER;
    pixel_y = (u32)local_y + HEIGHTMAP_BORDER;
    return true;
}

bool EditorTerrain::read_height(i32 world_x, i32 world_y, u8 &height) {
    u32 chunk_idx = 0;
    u32 px = 0;
    u32 py = 0;
    if (!world_to_heightmap_pixel(world_x, world_y, chunk_idx, px, py)) {
        return false;
    }

    EditorTile *tile = get_tile(chunk_idx);
    if (tile == nullptr || tile->heightmap.is_null()) return false;
    height = tile->heightmap->pixel(px, py)[1];
    return true;
}

bool EditorTerrain::write_height(i32 world_x, i32 world_y, i32 height,
                                 TerrainEdit &edit) {
    u32 chunk_idx = 0;
    u32 px = 0;
    u32 py = 0;
    if (!world_to_heightmap_pixel(world_x, world_y, chunk_idx, px, py)) {
        return false;
    }

    EditorTile *tile = get_tile(chunk_idx);
    if (tile == nullptr || tile->heightmap.is_null()) return false;

    u8 value = (u8)std::clamp(height, 0, 255);
    u8 *pixel = tile->heightmap->pixel(px, py);
    if (pixel[0] == value && pixel[1] == value) return false;

    pixel[0] = value;
    pixel[1] = value;
    edit.heightmaps.insert(chunk_idx);
    return true;
}

bool EditorTerrain::write_height(i32 world_x, i32 world_y, i32 height) {
    TerrainEdit edit;
    if (!write_height(world_x, world_y, height, edit)) return false;

    commit_edit(edit);
    return true;
}

bool EditorTerrain::write_controlmap(i32 world_x, i32 world_y, f32 amount,
                                     i32 palette_slot, TerrainEdit &edit) {
    if (palette_slot < 0) return false;

    u32 chunk_idx = 0;
    u32 px = 0;
    u32 py = 0;
    if (!world_to_heightmap_pixel(world_x, world_y, chunk_idx, px, py)) {
        return false;
    }

    EditorTile *tile = get_tile(chunk_idx);
    if (tile == nullptr || tile->controlmap.is_null()) return false;

    i32 delta = (i32)std::round(std::clamp(amount, 0.0f, 1.0f) * 255.0f);
    if (delta <= 0) return false;

    u8 slot = (u8)std::clamp(palette_slot, 0, 255);
    u8 *pixel = tile->controlmap->pixel(px, py);
    u8 tex1 = pixel[0];
    u8 tex2 = pixel[1];
    u8 blend = pixel[2];

    if (tex1 == slot) {
        pixel[2] = (u8)std::max(0, (i32)blend - delta);
    } else {
        if (tex2 != slot) {
            if (blend >= 128) {
                pixel[0] = tex2;
                blend = 0;
            }
            pixel[1] = slot;
        }
        pixel[2] = (u8)std::min(255, (i32)blend + delta);
    }

    if (pixel[0] == tex1 && pixel[1] == tex2 && pixel[2] == blend) {
        return false;
    }

    edit.controlmaps.insert(chunk_idx);
    return true;
}

bool EditorTerrain::write_controlmap(i32 world_x, i32 world_y, f32 amount,
                                     i32 palette_slot) {
    TerrainEdit edit;
    if (!write_controlmap(world_x, world_y, amount, palette_slot, edit)) {
        return false;
    }

    commit_edit(edit);
    return true;
}

bool EditorTerrain::apply_brush_sample(i32 x, i32 y, TerrainBrush type,
                                       f32 amount,
                                       const TerrainBrushSetting &setting,
                                       TerrainEdit &edit) {
    if (amount <= 0.0f) return false;

    u8 current = 0;
    switch (type) {
        case TerrainBrush::Raise:
            return read_height(x, y, current) &&
                   write_height(x, y,
                                current + (i32)std::round(255.0f * amount),
                                edit);
        case TerrainBrush::Lower:
            return read_height(x, y, current) &&
                   write_height(x, y,
                                current - (i32)std::round(255.0f * amount),
                                edit);
        case TerrainBrush::Flatten:
            return read_height(x, y, current) &&
                   write_height(
                       x, y,
                       current + (i32)std::round(
                                     ((i32)setting.flatten_height - current) *
                                     amount),
                       edit);
        case TerrainBrush::Smooth: {
            if (!read_height(x, y, current)) return false;
            i32 sum = 0;
            i32 count = 0;
            for (i32 sy = -1; sy <= 1; sy++) {
                for (i32 sx = -1; sx <= 1; sx++) {
                    u8 neighbor = 0;
                    if (read_height(x + sx, y + sy, neighbor)) {
                        sum += neighbor;
                        count++;
                    }
                }
            }
            if (count == 0) return false;
            f32 target = (f32)sum / (f32)count;
            return write_height(
                x, y,
                current + (i32)std::round((target - current) * amount),
                edit);
        }
        case TerrainBrush::Splat:
            return write_controlmap(x, y, amount, setting.terrain_palette_slot,
                                    edit);
        case TerrainBrush::Pick:
            return false;
    }
    return false;
}

bool EditorTerrain::apply_brush(i32 x, i32 y, TerrainBrush type,
                                const TerrainBrushSetting &setting) {
    if (type == TerrainBrush::Pick) return false;

    TerrainEdit edit;
    i32 radius = (i32)setting.radius;
    for (i32 dy = -radius; dy <= radius; dy++) {
        for (i32 dx = -radius; dx <= radius; dx++) {
            f32 dist = std::sqrt((f32)(dx * dx + dy * dy));
            if (dist > (f32)radius) continue;

            f32 falloff = 1.0f - dist / std::max(1.0f, (f32)radius);
            f32 amount = std::clamp((f32)setting.strength * 0.01f * falloff,
                                    0.0f, 1.0f);
            apply_brush_sample(x + dx, y + dy, type, amount, setting, edit);
        }
    }

    if (!edit.has_changes()) return false;
    commit_edit(edit);
    return true;
}

void EditorTerrain::rebuild_tile_border(u32 chunk_index,
                                        EditorTile::TileImage image_type) {
    EditorTile *tile = get_tile(chunk_index);
    if (tile == nullptr) return;

    tile->clamp_border(image_type);
    tile->build_border_from_tile(get_tile_at(tile->x - 1, tile->y),
                                 EditorTile::TileDirection::LEFT, image_type);
    tile->build_border_from_tile(get_tile_at(tile->x + 1, tile->y),
                                 EditorTile::TileDirection::RIGHT, image_type);
    tile->build_border_from_tile(get_tile_at(tile->x, tile->y - 1),
                                 EditorTile::TileDirection::BOTTOM,
                                 image_type);
    tile->build_border_from_tile(get_tile_at(tile->x, tile->y + 1),
                                 EditorTile::TileDirection::UP, image_type);
}

void EditorTerrain::sync_tile_neighbor(u32 chunk_index, i32 neighbor_index,
                                       EditorTile::TileDirection direction,
                                       EditorTile::TileImage image_type,
                                       std::set<u32> &touched_chunks) {
    if (neighbor_index < 0) return;

    EditorTile *tile = get_tile(chunk_index);
    EditorTile *neighbor = get_tile((u32)neighbor_index);
    if (tile == nullptr || neighbor == nullptr) return;

    bool copied = false;
    switch (direction) {
        case EditorTile::TileDirection::LEFT:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::TileDirection::RIGHT, image_type);
            break;
        case EditorTile::TileDirection::RIGHT:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::TileDirection::LEFT, image_type);
            break;
        case EditorTile::TileDirection::BOTTOM:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::TileDirection::UP, image_type);
            break;
        case EditorTile::TileDirection::UP:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::TileDirection::BOTTOM, image_type);
            break;
    }

    if (copied) touched_chunks.insert((u32)neighbor_index);
}

void EditorTerrain::sync_tile_seams(std::set<u32> &touched_chunks,
                                    EditorTile::TileImage image_type) {
    std::set<u32> source_chunks = touched_chunks;
    for (u32 chunk_index : source_chunks) {
        EditorTile *tile = get_tile(chunk_index);
        if (tile == nullptr) continue;

        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile->x - 1, tile->y),
                           EditorTile::TileDirection::LEFT, image_type,
                           touched_chunks);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile->x + 1, tile->y),
                           EditorTile::TileDirection::RIGHT, image_type,
                           touched_chunks);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile->x, tile->y - 1),
                           EditorTile::TileDirection::BOTTOM, image_type,
                           touched_chunks);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile->x, tile->y + 1),
                           EditorTile::TileDirection::UP, image_type,
                           touched_chunks);
    }

    std::set<u32> border_chunks = touched_chunks;
    for (u32 chunk_index : border_chunks) {
        rebuild_tile_border(chunk_index, image_type);
    }
}

void EditorTerrain::commit_edit(TerrainEdit &edit) {
    if (!edit.has_changes()) return;

    if (!edit.heightmaps.empty()) {
        sync_tile_seams(edit.heightmaps, EditorTile::TileImage::Heightmap);
    }
    if (!edit.controlmaps.empty()) {
        sync_tile_seams(edit.controlmaps, EditorTile::TileImage::Controlmap);
    }

    upload_heightmaps(edit.heightmaps);
    upload_controlmaps(edit.controlmaps);

    dirty_heightmaps.insert(edit.heightmaps.begin(), edit.heightmaps.end());
    dirty_controlmaps.insert(edit.controlmaps.begin(), edit.controlmaps.end());
}

void EditorTerrain::connect_chunk(u32 chunk_index) {
    EditorTile *tile = get_tile(chunk_index);
    if (tile == nullptr) return;

    tile->build_edge_from_tile(get_tile_at(tile->x - 1, tile->y),
                               EditorTile::TileDirection::LEFT);
    tile->build_edge_from_tile(get_tile_at(tile->x + 1, tile->y),
                               EditorTile::TileDirection::RIGHT);
    tile->build_edge_from_tile(get_tile_at(tile->x, tile->y - 1),
                               EditorTile::TileDirection::BOTTOM);
    tile->build_edge_from_tile(get_tile_at(tile->x, tile->y + 1),
                               EditorTile::TileDirection::UP);

    TerrainEdit edit;
    edit.heightmaps.insert(chunk_index);
    edit.controlmaps.insert(chunk_index);
    commit_edit(edit);
}

void EditorTerrain::sync_loaded_tile_seams() {
    std::set<u32> touched_heightmaps;
    std::set<u32> touched_controlmaps;
    for (const auto &entry : tiles) {
        u32 chunk_index = entry.first;
        const EditorTile &tile = entry.second;
        touched_heightmaps.insert(chunk_index);
        touched_controlmaps.insert(chunk_index);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile.x - 1, tile.y),
                           EditorTile::TileDirection::LEFT,
                           EditorTile::TileImage::Heightmap,
                           touched_heightmaps);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile.x, tile.y - 1),
                           EditorTile::TileDirection::BOTTOM,
                           EditorTile::TileImage::Heightmap,
                           touched_heightmaps);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile.x - 1, tile.y),
                           EditorTile::TileDirection::LEFT,
                           EditorTile::TileImage::Controlmap,
                           touched_controlmaps);
        sync_tile_neighbor(chunk_index, find_chunk_index_at(tile.x, tile.y - 1),
                           EditorTile::TileDirection::BOTTOM,
                           EditorTile::TileImage::Controlmap,
                           touched_controlmaps);
    }

    for (u32 chunk_index : touched_heightmaps) {
        rebuild_tile_border(chunk_index, EditorTile::TileImage::Heightmap);
    }
    for (u32 chunk_index : touched_controlmaps) {
        rebuild_tile_border(chunk_index, EditorTile::TileImage::Controlmap);
    }

    upload_heightmaps(touched_heightmaps);
    upload_controlmaps(touched_controlmaps);
}

void EditorTerrain::upload_heightmaps(const std::set<u32> &touched_chunks) {
    for (u32 chunk_idx : touched_chunks) {
        EditorTile *tile = get_tile(chunk_idx);
        if (tile == nullptr) continue;
        update_chunk_heightmap(chunk_idx, tile->heightmap);
    }
}

void EditorTerrain::upload_controlmaps(const std::set<u32> &touched_chunks) {
    for (u32 chunk_idx : touched_chunks) {
        EditorTile *tile = get_tile(chunk_idx);
        if (tile == nullptr) continue;
        update_chunk_controlmap(chunk_idx, tile->controlmap);
    }
}

void EditorTerrain::clear_dirty_maps() {
    dirty_heightmaps.clear();
    dirty_controlmaps.clear();
}

void EditorTerrain::save_dirty_maps(const std::vector<ChunkSetting> &chunks) {
    if (dirty_heightmaps.empty() && dirty_controlmaps.empty()) return;

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return;

    for (u32 chunk_idx : dirty_heightmaps) {
        if (chunk_idx >= chunks.size()) continue;

        ResourceEntry *entry =
            loader->get_entries().get_entry(chunks[chunk_idx].height_map);
        EditorTile *tile = get_tile(chunk_idx);
        if (entry == nullptr || tile == nullptr || tile->heightmap.is_null()) {
            continue;
        }
        tile->heightmap->save_disk(entry->real_path());
    }

    for (u32 chunk_idx : dirty_controlmaps) {
        if (chunk_idx >= chunks.size()) continue;

        ResourceEntry *entry =
            loader->get_entries().get_entry(chunks[chunk_idx].control_map);
        EditorTile *tile = get_tile(chunk_idx);
        if (entry == nullptr || tile == nullptr || tile->controlmap.is_null()) {
            continue;
        }
        tile->controlmap->save_disk(entry->real_path());
    }

    clear_dirty_maps();
}

EditorTerrain::~EditorTerrain() {}
}  // namespace Seed
