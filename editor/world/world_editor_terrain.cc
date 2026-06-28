#include "world_editor.h"
#include "editor/world/editor_world.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include "core/engine.h"
#include "core/project.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"

namespace Seed {
#define WORLD_EDITOR_CHUNK_SIZE (256)
#define WORLD_EDITOR_CHUNK_HALF (WORLD_EDITOR_CHUNK_SIZE / 2)
#define TILE_BORDER (1)

void WorldEditor::save_dirty_heightmaps() {
    if (current_world == nullptr || dirty_heightmaps.empty()) return;

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return;
    for (u32 chunk_idx : dirty_heightmaps) {
        if (chunk_idx >= current_world->get_chunks().size()) continue;

        ResourceEntry *entry = loader->get_entries().get_entry(
            current_world->get_chunks()[chunk_idx].height_map);
        EditorTile *tile = current_world->get_tile(chunk_idx);
        if (entry == nullptr || tile == nullptr || tile->heightmap.is_null()) {
            continue;
        }
        tile->heightmap->save_disk(entry->real_path());
    }
    dirty_heightmaps.clear();
    heightmaps_dirty = false;
}

void WorldEditor::save_dirty_controlmaps() {
    if (current_world == nullptr || dirty_controlmaps.empty()) return;

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return;
    for (u32 chunk_idx : dirty_controlmaps) {
        if (chunk_idx >= current_world->get_chunks().size()) continue;

        ResourceEntry *entry = loader->get_entries().get_entry(
            current_world->get_chunks()[chunk_idx].control_map);
        EditorTile *tile = current_world->get_tile(chunk_idx);
        if (entry == nullptr || tile == nullptr || tile->controlmap.is_null()) {
            continue;
        }
        tile->controlmap->save_disk(entry->real_path());
    }
    dirty_controlmaps.clear();
    controlmaps_dirty = false;
}

i32 WorldEditor::find_chunk_index_at_chunk(i32 chunk_x, i32 chunk_y) const {
    if (current_world == nullptr) return -1;

    std::map<std::pair<i32, i32>, u32>::iterator iter =
        current_world->pos_to_index.find({chunk_x, chunk_y});
    if (iter == current_world->pos_to_index.end()) return -1;
    return (i32)iter->second;
}

bool WorldEditor::chunk_exists_at(i32 chunk_x, i32 chunk_y) const {
    return find_chunk_index_at_chunk(chunk_x, chunk_y) >= 0;
}

std::string WorldEditor::static_model_label(UUID uuid) const {
    ResourceEntry *entry =
        ResourceLoader::get_instance()->get_entries().get_entry(uuid);
    if (entry == nullptr) return "Missing Model";
    KStr name = entry->path.filename();
    return std::string(name.data(), name.length());
}

void WorldEditor::select_static_object(u32 chunk_index, u32 object_index) {
    if (current_world == nullptr) return;
    selected_static_chunk = (i32)chunk_index;
    selected_static_object = (i32)object_index;
    gEditor->set_current_inspect(new EditorStaticObjectInspector(
        current_world, chunk_index, object_index));
}

bool WorldEditor::add_chunk_at(i32 chunk_x, i32 chunk_y) {
    if (current_world == nullptr) return false;
    if (chunk_exists_at(chunk_x, chunk_y)) {
        status_text = "Terrain tile already exists.";
        return false;
    }

    current_world->add_new_chunk(chunk_x, chunk_y);
    u32 chunk_idx = (u32)current_world->get_chunks().size() - 1;
    std::set<u32> touched_chunks = {chunk_idx};
    sync_tile_seams(touched_chunks);
    upload_touched_tiles(touched_chunks);
    for (u32 touched_idx : touched_chunks) {
        dirty_heightmaps.insert(touched_idx);
        dirty_controlmaps.insert(touched_idx);
    }
    heightmaps_dirty = !dirty_heightmaps.empty();
    controlmaps_dirty = !dirty_controlmaps.empty();
    save_current_world();

    status_text = fmt::format("Added terrain tile ({}, {}).", chunk_x, chunk_y);
    return true;
}

i32 WorldEditor::find_chunk_index_at_world(i32 world_x, i32 world_y) const {
    if (current_world == nullptr) return -1;

    const std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    i32 best_idx = -1;
    i32 best_score = -1;
    for (i32 i = 0; i < (i32)chunks.size(); i++) {
        i32 chunk_origin_x =
            chunks[i].x * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
        i32 chunk_origin_y =
            chunks[i].y * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
        i32 local_x = world_x - chunk_origin_x;
        i32 local_y = world_y - chunk_origin_y;
        if (local_x < 0 || local_y < 0 || local_x > WORLD_EDITOR_CHUNK_SIZE ||
            local_y > WORLD_EDITOR_CHUNK_SIZE) {
            continue;
        }

        i32 score = 0;
        if (local_x == WORLD_EDITOR_CHUNK_SIZE) score++;
        if (local_y == WORLD_EDITOR_CHUNK_SIZE) score++;
        if (score > best_score) {
            best_idx = i;
            best_score = score;
        }
    }
    return best_idx;
}

bool WorldEditor::world_to_heightmap_pixel(i32 world_x, i32 world_y,
                                           u32 &chunk_idx, u32 &pixel_x,
                                           u32 &pixel_y) const {
    i32 idx = find_chunk_index_at_world(world_x, world_y);
    if (idx < 0 || current_world == nullptr) return false;

    const ChunkSetting &chunk = current_world->get_chunks()[idx];
    i32 chunk_origin_x =
        (i32)chunk.x * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
    i32 chunk_origin_y =
        (i32)chunk.y * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
    i32 local_x = world_x - chunk_origin_x;
    i32 local_y = world_y - chunk_origin_y;
    if (local_x < 0 || local_y < 0 || local_x > WORLD_EDITOR_CHUNK_SIZE ||
        local_y > WORLD_EDITOR_CHUNK_SIZE) {
        return false;
    }

    chunk_idx = (u32)idx;
    pixel_x = (u32)local_x + TILE_BORDER;
    pixel_y = (u32)local_y + TILE_BORDER;
    return true;
}

bool WorldEditor::load_world(const UUID uuid) {
    status_text.clear();

    if (gEditor != nullptr && gEditor->ctx.current_inspect != nullptr) {
        gEditor->set_current_inspect(nullptr);
    }

    delete current_world;
    current_world = nullptr;
    current_entry =
        ResourceLoader::get_instance()->get_entries().get_entry(uuid);
    if (current_entry == nullptr) {
        status_text = "World file is not registered in resource entries.";
        return false;
    }

    current_world = new EditorWorld(current_entry);

    dirty_heightmaps.clear();
    dirty_controlmaps.clear();
    heightmaps_dirty = false;
    controlmaps_dirty = false;
    selected_terrain_palette_slot = -1;
    selected_static_chunk = -1;
    selected_static_object = -1;

    set_current_world_inspector();
    if (gEditor != nullptr) {
        gEditor->set_last_open_world(current_entry->uuid);
    }
    status_text = "World loaded.";
    return true;
}

void WorldEditor::save_current_world() {
    if (current_world == nullptr) return;
    save_dirty_heightmaps();
    save_dirty_controlmaps();
    current_world->save();

    Project *project = SeedEngine::get_instance()->get_project();
    if (project != nullptr) {
        ResourceLoader::get_instance()->get_entries().save(
            project->get_entry_path());
        status_text = "World saved through resource entries.";
    }
}

void WorldEditor::add_chunk() {
    if (current_world == nullptr) return;
    std::vector<ChunkSetting> chunks = current_world->get_chunks();
    if (chunks.empty()) {
        add_chunk_at(0, 0);
        return;
    }

    static const i32 dirs[][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    for (const ChunkSetting &chunk : chunks) {
        for (u32 i = 0; i < 4; i++) {
            i32 next_x = chunk.x + dirs[i][0];
            i32 next_y = chunk.y + dirs[i][1];
            if (!chunk_exists_at(next_x, next_y)) {
                add_chunk_at(next_x, next_y);
                return;
            }
        }
    }
}

void WorldEditor::clear_tiles() {
    if (current_world == nullptr) return;

    std::vector<UUID> tile_assets;
    tile_assets.reserve(current_world->get_chunks().size() * 2);
    for (const ChunkSetting &chunk : current_world->get_chunks()) {
        if (!chunk.height_map.is_null()) {
            tile_assets.push_back(chunk.height_map);
        }
        if (!chunk.control_map.is_null()) {
            tile_assets.push_back(chunk.control_map);
        }
    }

    for (UUID uuid : tile_assets) {
        gEditor->remove_asset(uuid);
    }

    current_world->clear_tiles();
    dirty_heightmaps.clear();
    dirty_controlmaps.clear();
    heightmaps_dirty = false;
    controlmaps_dirty = false;
    last_pick_valid = false;
    save_current_world();
    status_text = "All terrain tiles cleared.";
}

void WorldEditor::sync_tile_seams(std::set<u32> &touched_chunks) {
    if (current_world == nullptr) return;
    current_world->sync_tile_seams(touched_chunks);
}


void WorldEditor::upload_touched_tiles(const std::set<u32> &touched_chunks) {
    if (current_world == nullptr || current_world->terrain.is_null()) return;

    for (u32 chunk_idx : touched_chunks) {
        EditorTile *tile = current_world->get_tile(chunk_idx);
        if (tile == nullptr) continue;
        if (!tile->heightmap.is_null()) {
            current_world->terrain->update_chunk_heightmap(chunk_idx,
                                                           tile->heightmap);
        }
        if (!tile->controlmap.is_null()) {
            current_world->terrain->update_chunk_controlmap(chunk_idx,
                                                            tile->controlmap);
        }
    }
}

}  // namespace Seed
