#include "world_editor.h"
#include "editor/world/editor_world.h"
#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <imgui.h>
#include "core/engine.h"
#include "core/misc/type_name.h"
#include "core/misc/uuid.h"
#include "core/project.h"
#include "core/rendering/viewport.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"
#include "core/rendering/rhi/render_engine.h"
#include "editor/world/world_renderer.h"

namespace Seed {

WorldEditor::~WorldEditor() { delete current_world; }

void WorldEditor::init() {
    screen_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                          PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                        PixelFormat::D32, nullptr);
    picking_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                           PixelFormat::RGBA16I, nullptr);

    renderer = new WorldRenderer(screen_texture, screen_depth,
                                 ref_cast<Texture>(picking_texture));
    RenderEngine::get_instance()->register_renderer(1, renderer);
}

ResourceEntry *WorldEditor::find_entry_for_path(const Path &path) {
    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return nullptr;

    Path entry_path = path;
    Project *project = SeedEngine::get_instance()->get_project();
    if (project != nullptr && entry_path.is_absolute()) {
        entry_path = entry_path.relative(project->get_path());
    }

    UUID uuid = loader->get_entries().get_uuid(entry_path);
    if (uuid.is_null()) return nullptr;
    return loader->get_entries().get_entry(uuid);
}

void WorldEditor::set_current_world_inspector() {
    if (gEditor == nullptr || current_world == nullptr) return;
    gEditor->set_current_inspect(new EditorWorldInspector(current_world));
}

void WorldEditor::mark_preview_terrain_dirty() { preview_terrain_dirty = true; }

#define WORLD_EDITOR_CHUNK_SIZE (256)
#define WORLD_EDITOR_CHUNK_HALF (WORLD_EDITOR_CHUNK_SIZE / 2)
#define WORLD_EDITOR_CHUNK_EDGE_SNAP (12)
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

bool WorldEditor::sample_terrain_pick_pixel(u32 x, u32 y, u32 viewport_h,
                                            i32 &world_x, i32 &world_y) {
    if (picking_texture.is_null() || viewport_h == 0) return false;

    u32 flipped_y = viewport_h - std::min(y, viewport_h);
    i16 *coord = (i16 *)picking_texture->pixel_repeat(x, flipped_y);
    if (coord[2] == 0) return false;
    world_x = coord[0];
    world_y = coord[1];
    return true;
}

bool WorldEditor::sample_terrain_pick(ImVec2 viewport_origin, f32 viewport_w,
                                      f32 viewport_h, i32 &world_x,
                                      i32 &world_y) {
    if (picking_texture.is_null()) return false;

    ImVec2 mouse = ImGui::GetMousePos();
    if (mouse.x < viewport_origin.x || mouse.y < viewport_origin.y ||
        mouse.x >= viewport_origin.x + viewport_w ||
        mouse.y >= viewport_origin.y + viewport_h) {
        return false;
    }

    Viewport vp = Viewport{(u32)viewport_w, (u32)viewport_h};
    Vec2 vp_coord = vp.to_viewport_coord(
        Vec2{mouse.x - viewport_origin.x, mouse.y - viewport_origin.y});

    u32 x = (u32)vp_coord.x;
    u32 y = (u32)vp_coord.y;
    return sample_terrain_pick_pixel(x, y, (u32)viewport_h, world_x, world_y);
}

bool WorldEditor::find_nearest_terrain_pixel(u32 start_x, u32 start_y,
                                             u32 viewport_w, u32 viewport_h,
                                             i32 &world_x, i32 &world_y,
                                             u32 &hit_x, u32 &hit_y) {
    if (viewport_w == 0 || viewport_h == 0) return false;

    u32 max_radius = std::max(viewport_w, viewport_h);
    for (u32 radius = 1; radius <= max_radius; radius++) {
        i32 min_x = std::max(0, (i32)start_x - (i32)radius);
        i32 max_x = std::min((i32)viewport_w - 1, (i32)start_x + (i32)radius);
        i32 min_y = std::max(0, (i32)start_y - (i32)radius);
        i32 max_y = std::min((i32)viewport_h - 1, (i32)start_y + (i32)radius);

        for (i32 x = min_x; x <= max_x; x++) {
            if (sample_terrain_pick_pixel((u32)x, (u32)min_y, viewport_h,
                                          world_x, world_y)) {
                hit_x = (u32)x;
                hit_y = (u32)min_y;
                return true;
            }
            if (min_y != max_y &&
                sample_terrain_pick_pixel((u32)x, (u32)max_y, viewport_h,
                                          world_x, world_y)) {
                hit_x = (u32)x;
                hit_y = (u32)max_y;
                return true;
            }
        }

        for (i32 y = min_y + 1; y < max_y; y++) {
            if (sample_terrain_pick_pixel((u32)min_x, (u32)y, viewport_h,
                                          world_x, world_y)) {
                hit_x = (u32)min_x;
                hit_y = (u32)y;
                return true;
            }
            if (min_x != max_x &&
                sample_terrain_pick_pixel((u32)max_x, (u32)y, viewport_h,
                                          world_x, world_y)) {
                hit_x = (u32)max_x;
                hit_y = (u32)y;
                return true;
            }
        }
    }
    return false;
}

bool WorldEditor::add_chunk_from_empty_viewport_click(ImVec2 viewport_origin,
                                                      f32 viewport_w,
                                                      f32 viewport_h) {
    if (current_world == nullptr || !ImGui::IsItemHovered() ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return false;
    }

    ImVec2 mouse = ImGui::GetMousePos();
    if (mouse.x < viewport_origin.x || mouse.y < viewport_origin.y ||
        mouse.x >= viewport_origin.x + viewport_w ||
        mouse.y >= viewport_origin.y + viewport_h) {
        return false;
    }

    std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    if (chunks.empty()) {
        return add_chunk_at(0, 0);
    }

    Viewport vp = Viewport{(u32)viewport_w, (u32)viewport_h};
    Vec2 vp_coord = vp.to_viewport_coord(
        Vec2{mouse.x - viewport_origin.x, mouse.y - viewport_origin.y});

    u32 mouse_x = std::min((u32)viewport_w - 1, (u32)vp_coord.x);
    u32 mouse_y = std::min((u32)viewport_h - 1, (u32)vp_coord.y);
    i32 world_x = 0;
    i32 world_y = 0;
    if (sample_terrain_pick_pixel(mouse_x, mouse_y, (u32)viewport_h, world_x,
                                  world_y)) {
        return false;
    }

    u32 hit_x = 0;
    u32 hit_y = 0;
    if (!find_nearest_terrain_pixel(mouse_x, mouse_y, (u32)viewport_w,
                                    (u32)viewport_h, world_x, world_y, hit_x,
                                    hit_y)) {
        status_text = "No nearby terrain tile edge.";
        return false;
    }

    i32 chunk_idx = find_chunk_index_at_world(world_x, world_y);
    if (chunk_idx < 0) return false;

    const ChunkSetting &chunk = chunks[chunk_idx];
    i32 chunk_origin_x =
        chunk.x * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
    i32 chunk_origin_y =
        chunk.y * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
    i32 local_x = world_x - chunk_origin_x;
    i32 local_y = world_y - chunk_origin_y;
    if (local_x < 0) local_x = 0;
    if (local_x > WORLD_EDITOR_CHUNK_SIZE) local_x = WORLD_EDITOR_CHUNK_SIZE;
    if (local_y < 0) local_y = 0;
    if (local_y > WORLD_EDITOR_CHUNK_SIZE) local_y = WORLD_EDITOR_CHUNK_SIZE;

    i32 dist_left = local_x;
    i32 dist_right = WORLD_EDITOR_CHUNK_SIZE - local_x;
    i32 dist_bottom = local_y;
    i32 dist_top = WORLD_EDITOR_CHUNK_SIZE - local_y;
    i32 min_dist = std::min(std::min(dist_left, dist_right),
                            std::min(dist_bottom, dist_top));
    if (min_dist > WORLD_EDITOR_CHUNK_EDGE_SNAP) {
        status_text = "Click closer to a terrain tile edge.";
        return false;
    }

    i32 target_x = chunk.x;
    i32 target_y = chunk.y;
    if (min_dist == dist_left) {
        target_x--;
    } else if (min_dist == dist_right) {
        target_x++;
    } else if (min_dist == dist_bottom) {
        target_y--;
    } else {
        target_y++;
    }

    if (chunk_exists_at(target_x, target_y)) {
        status_text = "Terrain tile already exists.";
        return false;
    }

    return add_chunk_at(target_x, target_y);
}

const char *WorldEditor::terrain_tool_name(WorldTerrainTool tool) const {
    switch (tool) {
        case WorldTerrainTool::Raise:
            return "Raise";
        case WorldTerrainTool::Lower:
            return "Lower";
        case WorldTerrainTool::Smooth:
            return "Smooth";
        case WorldTerrainTool::Flatten:
            return "Flatten";
        case WorldTerrainTool::Pick:
            return "Pick";
        case WorldTerrainTool::Splat:
            return "Splat";
        default:
            return "Unknown";
    }
}

bool WorldEditor::is_texture_asset(UUID uuid) const {
    if (uuid.is_null()) return false;
    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return false;

    ResourceEntry *entry = loader->get_entries().get_entry(uuid);
    return entry != nullptr && entry->type_id == type_id<Texture>();
}

std::string WorldEditor::terrain_texture_label(UUID uuid) const {
    if (uuid.is_null()) return "Empty slot";

    ResourceLoader *loader = ResourceLoader::get_instance();
    if (loader == nullptr) return uuid.to_string();

    ResourceEntry *entry = loader->get_entries().get_entry(uuid);
    if (entry == nullptr) return uuid.to_string();

    KStr filename = entry->path.filename();
    if (filename.length() == 0) return uuid.to_string();
    return std::string(filename.data(), filename.length());
}

const char *WorldEditor::terrain_palette_map_label(
    TerrainPaletteMap map) const {
    switch (map) {
        case TerrainPaletteMap::Albedo:
            return "Albedo";
        case TerrainPaletteMap::Normal:
            return "Normal";
        default:
            return "Unknown";
    }
}

UUID WorldEditor::terrain_palette_uuid(u32 index, TerrainPaletteMap map) const {
    if (current_world == nullptr) return UUID{};

    if (map == TerrainPaletteMap::Albedo) {
        const std::vector<UUID> &textures =
            current_world->get_terrain_textures();
        return index < textures.size() ? textures[index] : UUID{};
    }

    const std::vector<UUID> &normals = current_world->get_terrain_normals();
    return index < normals.size() ? normals[index] : UUID{};
}

const EditorUI::TexturePreview *WorldEditor::terrain_palette_preview(
    u32 index, TerrainPaletteMap map) const {
    if (current_world == nullptr) return nullptr;
    if (map == TerrainPaletteMap::Albedo) {
        return current_world->get_terrain_texture_preview(index);
    }
    return current_world->get_terrain_normal_preview(index);
}

bool WorldEditor::set_terrain_palette_texture(u32 index, TerrainPaletteMap map,
                                              UUID texture) {
    if (current_world == nullptr) return false;
    if (map == TerrainPaletteMap::Albedo) {
        return current_world->set_terrain_texture(index, texture);
    }
    return current_world->set_terrain_normal(index, texture);
}

bool WorldEditor::accept_terrain_palette_drop(u32 index,
                                              TerrainPaletteMap map) {
    if (current_world == nullptr) return false;

    UUID texture = EditorUI::accept_uuid();
    if (texture.is_null()) return false;

    if (!is_texture_asset(texture)) {
        status_text = "Terrain palette accepts texture assets only.";
        return false;
    }

    bool uploaded = set_terrain_palette_texture(index, map, texture);
    selected_terrain_palette_slot = (i32)index;
    save_current_world();
    switch (map) {
        case TerrainPaletteMap::Albedo:
            status_text =
                uploaded ? "Terrain albedo assigned."
                         : "Albedo saved, but array upload requires 1024 x "
                           "1024 RGBA.";
        case TerrainPaletteMap::Normal:
            status_text =
                uploaded ? "Terrain normal assigned."
                         : "Normal saved, but array upload requires 1024 x "
                           "1024 RGB.";
        default:
            status_text = uploaded
                              ? "Terrain texture assigned."
                              : "Texture saved, but array upload was skipped.";
    }
    return true;
}
void WorldEditor::remove_terrain_palette_slot(u32 index) {
    if (current_world == nullptr) return;
    current_world->remove_terrain_texture(index);
    if (selected_terrain_palette_slot == (i32)index) {
        selected_terrain_palette_slot = -1;
    } else if (selected_terrain_palette_slot > (i32)index) {
        selected_terrain_palette_slot--;
    }
    save_current_world();
    status_text = "Terrain layer removed.";
}

bool WorldEditor::load_world(const Path &path) {
    status_text.clear();

    if (gEditor != nullptr && gEditor->ctx.current_inspect != nullptr) {
        gEditor->set_current_inspect(nullptr);
    }

    delete current_world;
    current_world = nullptr;
    current_entry = find_entry_for_path(path);
    if (current_entry == nullptr) {
        status_text = "World file is not registered in resource entries.";
        return false;
    }

    current_world = new EditorWorld(current_entry);
    current_world_path = current_entry->path;

    dirty_heightmaps.clear();
    dirty_controlmaps.clear();
    heightmaps_dirty = false;
    controlmaps_dirty = false;
    selected_terrain_palette_slot = -1;

    mark_preview_terrain_dirty();
    set_current_world_inspector();
    if (gEditor != nullptr) {
        gEditor->set_last_open_world(current_world_path);
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

bool WorldEditor::read_height(i32 world_x, i32 world_y, u8 &height) {
    u32 chunk_idx = 0;
    u32 px = 0;
    u32 py = 0;
    if (!world_to_heightmap_pixel(world_x, world_y, chunk_idx, px, py)) {
        return false;
    }

    EditorTile *tile = current_world->get_tile(chunk_idx);
    if (tile == nullptr || tile->heightmap.is_null()) return false;
    height = tile->heightmap->pixel(px, py)[1];
    return true;
}

bool WorldEditor::write_height(i32 world_x, i32 world_y, i32 height,
                               std::set<u32> &touched_chunks) {
    u32 chunk_idx = 0;
    u32 px = 0;
    u32 py = 0;
    if (!world_to_heightmap_pixel(world_x, world_y, chunk_idx, px, py)) {
        return false;
    }

    EditorTile *tile = current_world->get_tile(chunk_idx);
    if (tile == nullptr || tile->heightmap.is_null()) return false;

    if (height < 0) height = 0;
    if (height > 255) height = 255;
    tile->heightmap->pixel(px, py)[1] = (u8)height;
    touched_chunks.insert(chunk_idx);
    return true;
}

bool WorldEditor::write_controlmap(i32 world_x, i32 world_y, f32 amount,
                                   std::set<u32> &touched_chunks) {
    if (selected_terrain_palette_slot < 0) return false;
    if ((u32)selected_terrain_palette_slot >=
        current_world->get_terrain_textures().size()) {
        return false;
    }

    u32 chunk_idx = 0;
    u32 px = 0;
    u32 py = 0;
    if (!world_to_heightmap_pixel(world_x, world_y, chunk_idx, px, py)) {
        return false;
    }

    EditorTile *tile = current_world->get_tile(chunk_idx);
    if (tile == nullptr || tile->controlmap.is_null()) return false;

    u8 *pixel = tile->controlmap->pixel(px, py);
    u8 layer = (u8)selected_terrain_palette_slot;
    pixel[0] = layer;
    pixel[2] = 0;
    // i32 strength = pixel[1] + (i32)std::round(255.0f * amount);
    // if (strength > 255) strength = 255;
    // pixel[1] = (u8)strength;
    touched_chunks.insert(chunk_idx);
    return true;
}

bool WorldEditor::apply_raise_brush(i32 world_x, i32 world_y, f32 amount,
                                    std::set<u32> &touched_chunks) {
    u8 current = 0;
    if (!read_height(world_x, world_y, current)) return false;
    return write_height(world_x, world_y,
                        current + (i32)std::round(255.0f * amount),
                        touched_chunks);
}

bool WorldEditor::apply_lower_brush(i32 world_x, i32 world_y, f32 amount,
                                    std::set<u32> &touched_chunks) {
    u8 current = 0;
    if (!read_height(world_x, world_y, current)) return false;
    return write_height(world_x, world_y,
                        current - (i32)std::round(255.0f * amount),
                        touched_chunks);
}

bool WorldEditor::apply_flatten_brush(i32 world_x, i32 world_y, f32 amount,
                                      std::set<u32> &touched_chunks) {
    u8 current = 0;
    if (!read_height(world_x, world_y, current)) return false;

    i32 height = current + (i32)std::round((flatten_height - current) * amount);
    return write_height(world_x, world_y, height, touched_chunks);
}

bool WorldEditor::apply_smooth_brush(i32 world_x, i32 world_y, f32 amount,
                                     std::set<u32> &touched_chunks) {
    u8 current = 0;
    if (!read_height(world_x, world_y, current)) return false;

    i32 sum = 0;
    i32 count = 0;
    for (i32 sy = -1; sy <= 1; sy++) {
        for (i32 sx = -1; sx <= 1; sx++) {
            u8 neighbor = 0;
            if (read_height(world_x + sx, world_y + sy, neighbor)) {
                sum += neighbor;
                count++;
            }
        }
    }
    if (count == 0) return false;

    f32 target = (f32)sum / (f32)count;
    i32 height = current + (i32)std::round((target - current) * amount);
    return write_height(world_x, world_y, height, touched_chunks);
}

bool WorldEditor::apply_splat_brush(i32 world_x, i32 world_y, f32 amount,
                                    std::set<u32> &touched_chunks) {
    return write_controlmap(world_x, world_y, amount, touched_chunks);
}

bool WorldEditor::apply_brush_sample(i32 world_x, i32 world_y, f32 amount,
                                     std::set<u32> &touched_chunks) {
    switch (terrain_tool) {
        case WorldTerrainTool::Raise:
            return apply_raise_brush(world_x, world_y, amount, touched_chunks);
        case WorldTerrainTool::Lower:
            return apply_lower_brush(world_x, world_y, amount, touched_chunks);
        case WorldTerrainTool::Flatten:
            return apply_flatten_brush(world_x, world_y, amount,
                                       touched_chunks);
        case WorldTerrainTool::Smooth:
            return apply_smooth_brush(world_x, world_y, amount, touched_chunks);
        case WorldTerrainTool::Splat:
            return apply_splat_brush(world_x, world_y, amount, touched_chunks);
        case WorldTerrainTool::Pick:
            return false;
    }
    return false;
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

void WorldEditor::apply_terrain_brush(i32 world_x, i32 world_y) {
    if (current_world == nullptr || current_world->terrain.is_null()) return;

    i32 radius = std::max(1, (i32)brush_radius);
    std::set<u32> touched_chunks;

    for (i32 dy = -radius; dy <= radius; dy++) {
        for (i32 dx = -radius; dx <= radius; dx++) {
            f32 dist = std::sqrt((f32)(dx * dx + dy * dy));
            if (dist > brush_radius) continue;

            f32 falloff = 1.0f - dist / std::max(1.0f, brush_radius);
            apply_brush_sample(world_x + dx, world_y + dy,
                               brush_strength * falloff, touched_chunks);
        }
    }

    if (touched_chunks.empty()) return;

    sync_tile_seams(touched_chunks);
    upload_touched_tiles(touched_chunks);

    for (u32 chunk_idx : touched_chunks) {
        dirty_heightmaps.insert(chunk_idx);
        dirty_controlmaps.insert(chunk_idx);
    }
    heightmaps_dirty = true;
    controlmaps_dirty = true;
}

}  // namespace Seed