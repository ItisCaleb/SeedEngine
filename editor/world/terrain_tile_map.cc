#include "terrain_tile_map.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "core/world/terrain.h"

namespace Seed {

namespace {

constexpr i32 kTileSize = HEIGHTMAP_INNER_SIZE;
constexpr i32 kTileFirst = HEIGHTMAP_BORDER;
constexpr i32 kTileLast = kTileFirst + kTileSize - 1;
constexpr i32 kTilePadFirst = 0;
constexpr i32 kTilePadLast = HEIGHTMAP_SIZE - 1;

i32 floor_divide(i32 value, i32 divisor) {
    i32 result = value / divisor;
    if (value % divisor < 0) result--;
    return result;
}

}  // namespace

void EditorTile::clamp_border(Ref<Image> image) {
    if (image.is_null()) return;

    image->copy_column(image, kTileFirst, kTileFirst, kTilePadFirst, kTileFirst,
                       kTileSize);
    image->copy_column(image, kTileLast, kTileFirst, kTilePadLast, kTileFirst,
                       kTileSize);
    image->copy_row(image, kTilePadFirst, kTileFirst, kTilePadFirst,
                    kTilePadFirst, HEIGHTMAP_SIZE);
    image->copy_row(image, kTilePadFirst, kTileLast, kTilePadFirst,
                    kTilePadLast, HEIGHTMAP_SIZE);
}

Ref<Image> EditorTile::get_image(ImageType image_type) const {
    switch (image_type) {
        case ImageType::Heightmap:
            return heightmap;
        case ImageType::Controlmap:
            return controlmap;
    }
    return Ref<Image>();
}

void EditorTile::clamp_border(ImageType image_type) {
    clamp_border(get_image(image_type));
}

bool EditorTile::build_edge_from_image(Ref<Image> image, Ref<Image> source,
                                       Direction direction) {
    if (image.is_null() || source.is_null()) return false;

    switch (direction) {
        case Direction::LEFT:
            return source->copy_column(image, kTileLast, kTileFirst, kTileFirst,
                                       kTileFirst, kTileSize);
        case Direction::RIGHT:
            return source->copy_column(image, kTileFirst, kTileFirst, kTileLast,
                                       kTileFirst, kTileSize);
        case Direction::BOTTOM:
            return source->copy_row(image, kTileFirst, kTileLast, kTileFirst,
                                    kTileFirst, kTileSize);
        case Direction::UP:
            return source->copy_row(image, kTileFirst, kTileFirst, kTileFirst,
                                    kTileLast, kTileSize);
    }
    return false;
}

bool EditorTile::build_edge_from_tile(EditorTile *tile, Direction direction) {
    if (tile == nullptr) return false;

    const bool copied_heightmap =
        build_edge_from_image(heightmap, tile->heightmap, direction);
    const bool copied_controlmap =
        build_edge_from_image(controlmap, tile->controlmap, direction);
    return copied_heightmap || copied_controlmap;
}

bool EditorTile::build_edge_from_tile(EditorTile *tile, Direction direction,
                                      ImageType image_type) {
    if (tile == nullptr) return false;
    return build_edge_from_image(get_image(image_type),
                                 tile->get_image(image_type), direction);
}

bool EditorTile::build_border_from_image(Ref<Image> image, Ref<Image> source,
                                         Direction direction) {
    if (image.is_null() || source.is_null()) return false;

    switch (direction) {
        case Direction::LEFT:
            return source->copy_column(image, kTileLast - 1, kTileFirst,
                                       kTilePadFirst, kTileFirst, kTileSize);
        case Direction::RIGHT:
            return source->copy_column(image, kTileFirst + 1, kTileFirst,
                                       kTilePadLast, kTileFirst, kTileSize);
        case Direction::BOTTOM:
            return source->copy_row(image, kTileFirst, kTileLast - 1,
                                    kTileFirst, kTilePadFirst, kTileSize);
        case Direction::UP:
            return source->copy_row(image, kTileFirst, kTileFirst + 1,
                                    kTileFirst, kTilePadLast, kTileSize);
    }
    return false;
}

bool EditorTile::build_border_from_tile(EditorTile *tile, Direction direction,
                                        ImageType image_type) {
    if (tile == nullptr) return false;
    return build_border_from_image(get_image(image_type),
                                   tile->get_image(image_type), direction);
}

u32 TerrainTileMap::add_tile(i32 x, i32 y, Ref<Image> heightmap,
                             Ref<Image> controlmap) {
    const u32 tile_index = next_index++;
    tiles[tile_index] = EditorTile{
        .x = x, .y = y, .heightmap = heightmap, .controlmap = controlmap};
    position_to_index[{x, y}] = tile_index;
    return tile_index;
}

void TerrainTileMap::clear() {
    next_index = 0;
    position_to_index.clear();
    tiles.clear();
    clear_dirty_maps();
}

i32 TerrainTileMap::find_tile_index_at(i32 x, i32 y) const {
    auto iter = position_to_index.find({x, y});
    return iter == position_to_index.end() ? -1 : (i32)iter->second;
}

bool TerrainTileMap::tile_exists_at(i32 x, i32 y) const {
    return find_tile_index_at(x, y) >= 0;
}

EditorTile *TerrainTileMap::get_tile_at(i32 x, i32 y) {
    const i32 index = find_tile_index_at(x, y);
    return index < 0 ? nullptr : get_tile((u32)index);
}

EditorTile *TerrainTileMap::get_tile(u32 index) {
    auto iter = tiles.find(index);
    return iter == tiles.end() ? nullptr : &iter->second;
}

const EditorTile *TerrainTileMap::get_tile(u32 index) const {
    auto iter = tiles.find(index);
    return iter == tiles.end() ? nullptr : &iter->second;
}

i32 TerrainTileMap::find_tile_index(i32 world_x, i32 world_y) const {
    // A point can touch at most four chunks at a shared edge or corner.
    const i32 base_x = floor_divide(world_x + CHUNK_SIZE / 2, CHUNK_SIZE);
    const i32 base_y = floor_divide(world_y + CHUNK_SIZE / 2, CHUNK_SIZE);
    const std::array<i32, 2> candidate_x = {base_x, base_x - 1};
    const std::array<i32, 2> candidate_y = {base_y, base_y - 1};

    i32 best_index = -1;
    i32 best_score = -1;
    for (i32 tile_y : candidate_y) {
        for (i32 tile_x : candidate_x) {
            const i32 index = find_tile_index_at(tile_x, tile_y);
            if (index < 0) continue;

            const i32 local_x =
                world_x - (tile_x * CHUNK_SIZE - CHUNK_SIZE / 2);
            const i32 local_y =
                world_y - (tile_y * CHUNK_SIZE - CHUNK_SIZE / 2);
            if (local_x < 0 || local_y < 0 || local_x > CHUNK_SIZE ||
                local_y > CHUNK_SIZE) {
                continue;
            }

            const i32 score = (local_x == CHUNK_SIZE ? 1 : 0) +
                              (local_y == CHUNK_SIZE ? 1 : 0);
            if (score > best_score ||
                (score == best_score &&
                 (best_index < 0 || index < best_index))) {
                best_index = index;
                best_score = score;
            }
        }
    }
    return best_index;
}

bool TerrainTileMap::world_to_pixel(i32 world_x, i32 world_y, u32 &tile_index,
                                    u32 &pixel_x, u32 &pixel_y) const {
    const i32 index = find_tile_index(world_x, world_y);
    if (index < 0) return false;

    const EditorTile *tile = get_tile((u32)index);
    if (tile == nullptr) return false;

    const i32 local_x = world_x - (tile->x * CHUNK_SIZE - CHUNK_SIZE / 2);
    const i32 local_y = world_y - (tile->y * CHUNK_SIZE - CHUNK_SIZE / 2);
    if (local_x < 0 || local_y < 0 || local_x > CHUNK_SIZE ||
        local_y > CHUNK_SIZE) {
        return false;
    }

    tile_index = (u32)index;
    pixel_x = (u32)local_x + HEIGHTMAP_BORDER;
    pixel_y = (u32)local_y + HEIGHTMAP_BORDER;
    return true;
}

bool TerrainTileMap::read_height(i32 world_x, i32 world_y, u8 &height) const {
    u32 tile_index = 0;
    u32 pixel_x = 0;
    u32 pixel_y = 0;
    if (!world_to_pixel(world_x, world_y, tile_index, pixel_x, pixel_y)) {
        return false;
    }

    const EditorTile *tile = get_tile(tile_index);
    if (tile == nullptr || tile->heightmap.is_null()) return false;
    height = tile->heightmap->pixel(pixel_x, pixel_y)[1];
    return true;
}

bool TerrainTileMap::write_height(i32 world_x, i32 world_y, i32 height,
                                  TerrainEdit &edit) {
    u32 tile_index = 0;
    u32 pixel_x = 0;
    u32 pixel_y = 0;
    if (!world_to_pixel(world_x, world_y, tile_index, pixel_x, pixel_y)) {
        return false;
    }

    EditorTile *tile = get_tile(tile_index);
    if (tile == nullptr || tile->heightmap.is_null()) return false;

    const u8 value = (u8)std::clamp(height, 0, 255);
    u8 *pixel = tile->heightmap->pixel(pixel_x, pixel_y);
    if (pixel[0] == value && pixel[1] == value) return false;

    pixel[0] = value;
    pixel[1] = value;
    edit.heightmaps.insert(tile_index);
    return true;
}

bool TerrainTileMap::write_controlmap(i32 world_x, i32 world_y, f32 amount,
                                      i32 palette_slot, TerrainEdit &edit) {
    if (palette_slot < 0) return false;

    u32 tile_index = 0;
    u32 pixel_x = 0;
    u32 pixel_y = 0;
    if (!world_to_pixel(world_x, world_y, tile_index, pixel_x, pixel_y)) {
        return false;
    }

    EditorTile *tile = get_tile(tile_index);
    if (tile == nullptr || tile->controlmap.is_null()) return false;

    const i32 delta = (i32)std::round(std::clamp(amount, 0.0f, 1.0f) * 255.0f);
    if (delta <= 0) return false;

    const u8 slot = (u8)std::clamp(palette_slot, 0, 255);
    u8 *pixel = tile->controlmap->pixel(pixel_x, pixel_y);
    const u8 old_texture = pixel[0];
    const u8 old_blend_texture = pixel[1];
    const u8 old_blend = pixel[2];
    u8 blend = old_blend;

    if (old_texture == slot) {
        pixel[2] = (u8)std::max(0, (i32)blend - delta);
    } else {
        if (old_blend_texture != slot) {
            if (blend >= 128) {
                pixel[0] = old_blend_texture;
                blend = 0;
            }
            pixel[1] = slot;
        }
        pixel[2] = (u8)std::min(255, (i32)blend + delta);
    }

    if (pixel[0] == old_texture && pixel[1] == old_blend_texture &&
        pixel[2] == old_blend) {
        return false;
    }

    edit.controlmaps.insert(tile_index);
    return true;
}

void TerrainTileMap::rebuild_border(u32 tile_index,
                                    EditorTile::ImageType image_type) {
    EditorTile *tile = get_tile(tile_index);
    if (tile == nullptr) return;

    tile->clamp_border(image_type);
    tile->build_border_from_tile(get_tile_at(tile->x - 1, tile->y),
                                 EditorTile::Direction::LEFT, image_type);
    tile->build_border_from_tile(get_tile_at(tile->x + 1, tile->y),
                                 EditorTile::Direction::RIGHT, image_type);
    tile->build_border_from_tile(get_tile_at(tile->x, tile->y - 1),
                                 EditorTile::Direction::BOTTOM, image_type);
    tile->build_border_from_tile(get_tile_at(tile->x, tile->y + 1),
                                 EditorTile::Direction::UP, image_type);
}

void TerrainTileMap::sync_neighbor(u32 tile_index, i32 neighbor_index,
                                   EditorTile::Direction direction,
                                   EditorTile::ImageType image_type,
                                   std::set<u32> &changed_tiles) {
    if (neighbor_index < 0) return;

    EditorTile *tile = get_tile(tile_index);
    EditorTile *neighbor = get_tile((u32)neighbor_index);
    if (tile == nullptr || neighbor == nullptr) return;

    bool copied = false;
    switch (direction) {
        case EditorTile::Direction::LEFT:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::Direction::RIGHT, image_type);
            break;
        case EditorTile::Direction::RIGHT:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::Direction::LEFT, image_type);
            break;
        case EditorTile::Direction::BOTTOM:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::Direction::UP, image_type);
            break;
        case EditorTile::Direction::UP:
            copied = neighbor->build_edge_from_tile(
                tile, EditorTile::Direction::BOTTOM, image_type);
            break;
    }

    if (copied) changed_tiles.insert((u32)neighbor_index);
}

void TerrainTileMap::sync_seams(std::set<u32> &changed_tiles,
                                EditorTile::ImageType image_type) {
    const std::set<u32> source_tiles = changed_tiles;
    for (u32 tile_index : source_tiles) {
        EditorTile *tile = get_tile(tile_index);
        if (tile == nullptr) continue;

        sync_neighbor(tile_index, find_tile_index_at(tile->x - 1, tile->y),
                      EditorTile::Direction::LEFT, image_type, changed_tiles);
        sync_neighbor(tile_index, find_tile_index_at(tile->x + 1, tile->y),
                      EditorTile::Direction::RIGHT, image_type, changed_tiles);
        sync_neighbor(tile_index, find_tile_index_at(tile->x, tile->y - 1),
                      EditorTile::Direction::BOTTOM, image_type, changed_tiles);
        sync_neighbor(tile_index, find_tile_index_at(tile->x, tile->y + 1),
                      EditorTile::Direction::UP, image_type, changed_tiles);
    }

    const std::set<u32> border_tiles = changed_tiles;
    for (u32 tile_index : border_tiles) {
        rebuild_border(tile_index, image_type);
    }
}

void TerrainTileMap::commit(TerrainEdit &edit) {
    if (!edit.heightmaps.empty()) {
        sync_seams(edit.heightmaps, EditorTile::ImageType::Heightmap);
        dirty_heightmaps.insert(edit.heightmaps.begin(), edit.heightmaps.end());
    }
    if (!edit.controlmaps.empty()) {
        sync_seams(edit.controlmaps, EditorTile::ImageType::Controlmap);
        dirty_controlmaps.insert(edit.controlmaps.begin(),
                                 edit.controlmaps.end());
    }
}

TerrainEdit TerrainTileMap::connect_tile(u32 tile_index) {
    TerrainEdit edit;
    EditorTile *tile = get_tile(tile_index);
    if (tile == nullptr) return edit;

    tile->build_edge_from_tile(get_tile_at(tile->x - 1, tile->y),
                               EditorTile::Direction::LEFT);
    tile->build_edge_from_tile(get_tile_at(tile->x + 1, tile->y),
                               EditorTile::Direction::RIGHT);
    tile->build_edge_from_tile(get_tile_at(tile->x, tile->y - 1),
                               EditorTile::Direction::BOTTOM);
    tile->build_edge_from_tile(get_tile_at(tile->x, tile->y + 1),
                               EditorTile::Direction::UP);

    edit.heightmaps.insert(tile_index);
    edit.controlmaps.insert(tile_index);
    commit(edit);
    return edit;
}

TerrainEdit TerrainTileMap::synchronize_loaded_tiles() {
    TerrainEdit edit;
    for (const auto &entry : tiles) {
        const u32 tile_index = entry.first;
        const EditorTile &tile = entry.second;
        edit.heightmaps.insert(tile_index);
        edit.controlmaps.insert(tile_index);
        sync_neighbor(tile_index, find_tile_index_at(tile.x - 1, tile.y),
                      EditorTile::Direction::LEFT,
                      EditorTile::ImageType::Heightmap, edit.heightmaps);
        sync_neighbor(tile_index, find_tile_index_at(tile.x, tile.y - 1),
                      EditorTile::Direction::BOTTOM,
                      EditorTile::ImageType::Heightmap, edit.heightmaps);
        sync_neighbor(tile_index, find_tile_index_at(tile.x - 1, tile.y),
                      EditorTile::Direction::LEFT,
                      EditorTile::ImageType::Controlmap, edit.controlmaps);
        sync_neighbor(tile_index, find_tile_index_at(tile.x, tile.y - 1),
                      EditorTile::Direction::BOTTOM,
                      EditorTile::ImageType::Controlmap, edit.controlmaps);
    }

    for (u32 tile_index : edit.heightmaps) {
        rebuild_border(tile_index, EditorTile::ImageType::Heightmap);
    }
    for (u32 tile_index : edit.controlmaps) {
        rebuild_border(tile_index, EditorTile::ImageType::Controlmap);
    }
    return edit;
}

void TerrainTileMap::clear_dirty_maps() {
    dirty_heightmaps.clear();
    dirty_controlmaps.clear();
}

}  // namespace Seed
