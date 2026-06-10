#include "world_editor.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <nfd.h>
#include "core/engine.h"
#include "core/io/file.h"
#include "core/misc/uuid.h"
#include "core/project.h"
#include "core/rendering/viewport.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"
#include "core/rendering/rhi/render_engine.h"
#include "editor/world/world_renderer.h"

namespace Seed {

static void world_editor_section(const char *label) {
    ImGui::Spacing();
    ImGui::TextUnformatted(label);
    ImGui::Separator();
    ImGui::Spacing();
}

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
    gEditor->set_current_inspect(new EditorWorldInspector(current_world.get()));
}

void WorldEditor::mark_preview_terrain_dirty() { preview_terrain_dirty = true; }

static constexpr i32 WORLD_EDITOR_CHUNK_SIZE = 256;
static constexpr i32 WORLD_EDITOR_CHUNK_HALF = WORLD_EDITOR_CHUNK_SIZE / 2;
static constexpr i32 WORLD_EDITOR_CHUNK_EDGE_SNAP = 12;
static constexpr u32 WORLD_EDITOR_HEIGHTMAP_SIZE = 257;
static constexpr u32 WORLD_EDITOR_HEIGHTMAP_LAST =
    WORLD_EDITOR_HEIGHTMAP_SIZE - 1;

void WorldEditor::save_dirty_heightmaps() {
    if (current_world == nullptr || dirty_heightmaps.empty()) return;

    ResourceLoader *loader = ResourceLoader::get_instance();
    for (u32 chunk_idx : dirty_heightmaps) {
        if (chunk_idx >= current_world->chunks.size() ||
            chunk_idx >= current_world->heightmaps.size()) {
            continue;
        }
        ResourceEntry *entry =
            loader->get_entries().get_entry(current_world->chunks[chunk_idx].height_map);
        if (entry == nullptr || current_world->heightmaps[chunk_idx].is_null()) {
            continue;
        }
        current_world->heightmaps[chunk_idx]->save_disk(entry->real_path());
    }
    dirty_heightmaps.clear();
    heightmaps_dirty = false;
}

i32 WorldEditor::find_chunk_index_at_chunk(i32 chunk_x, i32 chunk_y) const {
    if (current_world == nullptr) return -1;

    const auto &chunks = current_world->get_chunks();
    for (i32 i = 0; i < (i32)chunks.size(); i++) {
        if (chunks[i].x == chunk_x && chunks[i].y == chunk_y) return i;
    }
    return -1;
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
    save_current_world();

    char buffer[96] = {};
    std::snprintf(buffer, sizeof(buffer), "Added terrain tile (%d, %d).",
                  chunk_x, chunk_y);
    status_text = buffer;
    return true;
}

i32 WorldEditor::find_chunk_index_at_world(i32 world_x, i32 world_y) const {
    if (current_world == nullptr) return -1;

    i32 chunk_x = (i32)std::floor(
        (world_x + WORLD_EDITOR_CHUNK_HALF) / (f32)WORLD_EDITOR_CHUNK_SIZE);
    i32 chunk_y = (i32)std::floor(
        (world_y + WORLD_EDITOR_CHUNK_HALF) / (f32)WORLD_EDITOR_CHUNK_SIZE);

    return find_chunk_index_at_chunk(chunk_x, chunk_y);
}

bool WorldEditor::world_to_heightmap_pixel(i32 world_x, i32 world_y,
                                           u32 &chunk_idx, u32 &pixel_x,
                                           u32 &pixel_y) const {
    i32 idx = find_chunk_index_at_world(world_x, world_y);
    if (idx < 0 || current_world == nullptr) return false;

    const EditorChunk &chunk = current_world->get_chunks()[idx];
    i32 chunk_origin_x = (i32)chunk.x * WORLD_EDITOR_CHUNK_SIZE -
                         WORLD_EDITOR_CHUNK_HALF;
    i32 chunk_origin_y = (i32)chunk.y * WORLD_EDITOR_CHUNK_SIZE -
                         WORLD_EDITOR_CHUNK_HALF;
    i32 local_x = world_x - chunk_origin_x;
    i32 local_y = world_y - chunk_origin_y;
    if (local_x < 0 || local_y < 0 || local_x > WORLD_EDITOR_CHUNK_SIZE ||
        local_y > WORLD_EDITOR_CHUNK_SIZE) {
        return false;
    }

    chunk_idx = (u32)idx;
    pixel_x = (u32)local_x;
    pixel_y = (u32)local_y;
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

bool WorldEditor::sample_terrain_pick(ImVec2 viewport_origin, float viewport_w,
                                      float viewport_h, i32 &world_x,
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
        i32 max_x = std::min((i32)viewport_w - 1,
                             (i32)start_x + (i32)radius);
        i32 min_y = std::max(0, (i32)start_y - (i32)radius);
        i32 max_y = std::min((i32)viewport_h - 1,
                             (i32)start_y + (i32)radius);

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
                                                     float viewport_w,
                                                     float viewport_h) {
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

    auto &chunks = current_world->get_chunks();
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

    const EditorChunk &chunk = chunks[chunk_idx];
    i32 chunk_origin_x =
        chunk.x * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
    i32 chunk_origin_y =
        chunk.y * WORLD_EDITOR_CHUNK_SIZE - WORLD_EDITOR_CHUNK_HALF;
    i32 local_x = std::clamp(world_x - chunk_origin_x, 0,
                             WORLD_EDITOR_CHUNK_SIZE);
    i32 local_y = std::clamp(world_y - chunk_origin_y, 0,
                             WORLD_EDITOR_CHUNK_SIZE);

    i32 dist_left = local_x;
    i32 dist_right = WORLD_EDITOR_CHUNK_SIZE - local_x;
    i32 dist_bottom = local_y;
    i32 dist_top = WORLD_EDITOR_CHUNK_SIZE - local_y;
    i32 min_dist =
        std::min(std::min(dist_left, dist_right),
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
        default:
            return "Unknown";
    }
}

bool WorldEditor::load_world(const Path &path) {
    status_text.clear();

    if (gEditor != nullptr && gEditor->ctx.current_inspect != nullptr) {
        gEditor->set_current_inspect(nullptr);
    }

    current_world.reset();
    current_entry = find_entry_for_path(path);
    current_world_from_entry = current_entry != nullptr;

    if (current_entry != nullptr) {
        current_world = std::make_unique<EditorWorld>(&current_entry->config);
        current_world_path = current_entry->path;
    } else {
        Ref<File> file = File::open(path, "rb");
        if (file.is_null()) {
            status_text = "Failed to open world file.";
            return false;
        }

        standalone_config = ResourceConfiguration(file->read_json());
        current_world = std::make_unique<EditorWorld>(&standalone_config);
        current_world_path = path;
    }

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
    current_world->save();

    Project *project = SeedEngine::get_instance()->get_project();
    if (current_world_from_entry && project != nullptr) {
        ResourceLoader::get_instance()->get_entries().save(
            project->get_entry_path());
        status_text = "World saved through resource entries.";
        return;
    }

    if (!current_world_path.to_str().is_empty()) {
        Ref<File> file = File::open(current_world_path, "wb");
        if (file.is_null()) {
            status_text = "Failed to save world file.";
            return;
        }
        file->write_str(standalone_config.get_json().dump(2));
        status_text = "World saved.";
    }
}

void WorldEditor::add_chunk() {
    if (current_world == nullptr) return;
    std::vector<EditorChunk> chunks = current_world->get_chunks();
    if (chunks.empty()) {
        add_chunk_at(0, 0);
        return;
    }

    static const i32 dirs[][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    for (const EditorChunk &chunk : chunks) {
        for (const auto &dir : dirs) {
            i32 next_x = chunk.x + dir[0];
            i32 next_y = chunk.y + dir[1];
            if (!chunk_exists_at(next_x, next_y)) {
                add_chunk_at(next_x, next_y);
                return;
            }
        }
    }
}

void WorldEditor::clear_tiles() {
    if (current_world == nullptr) return;

    std::vector<UUID> heightmap_assets;
    heightmap_assets.reserve(current_world->chunks.size());
    for (const EditorChunk &chunk : current_world->chunks) {
        UUID height_map = chunk.height_map;
        if (!height_map.is_null()) {
            heightmap_assets.push_back(height_map);
        }
    }

    for (UUID uuid : heightmap_assets) {
        gEditor->remove_asset(uuid);
    }

    current_world->clear_tiles();
    dirty_heightmaps.clear();
    heightmaps_dirty = false;
    last_pick_valid = false;
    save_current_world();
    status_text = "All terrain tiles cleared.";
}

void WorldEditor::draw_clear_tiles_confirmation_popup() {
    if (ImGui::BeginPopupModal("Clear Terrain Tiles?", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "This removes every terrain tile from the current world.");
        ImGui::TextDisabled("Heightmap assets referenced by those tiles are deleted.");
        ImGui::Separator();

        if (ImGui::Button("Confirm", ImVec2(120, 0))) {
            clear_tiles();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void WorldEditor::draw_vec3_field(const char *label, Vec3 &value) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::DragFloat3("##value", value.coord, 0.05f);
    ImGui::PopID();
}

void WorldEditor::draw_left_panel() {
    world_editor_section("World");

    if (current_world != nullptr) {
        if (ImGui::Button("Save World", ImVec2(-1, 0))) {
            save_current_world();
        }
        if (ImGui::Button("Inspect World", ImVec2(-1, 0))) {
            set_current_world_inspector();
        }

        ImGui::Spacing();
        ImGui::TextWrapped("%s", current_world_path.data());
    }

    if (!status_text.empty()) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", status_text.c_str());
    }

    world_editor_section("Mode");

    if (ImGui::Selectable("World", active_mode == WorldEditorMode::World)) {
        active_mode = WorldEditorMode::World;
    }
    if (ImGui::Selectable("Terrain",
                          active_mode == WorldEditorMode::Terrain)) {
        active_mode = WorldEditorMode::Terrain;
    }

    world_editor_section("Terrain");

    if (current_world == nullptr) {
        ImGui::TextDisabled("No world loaded.");
        return;
    }

    if (ImGui::Button("Add Terrain Tile", ImVec2(-1, 0))) {
        add_chunk();
    }

    bool has_tiles = !current_world->get_chunks().empty();
    if (!has_tiles) ImGui::BeginDisabled();
    if (ImGui::Button("Clear Terrain Tiles", ImVec2(-1, 0))) {
        ImGui::OpenPopup("Clear Terrain Tiles?");
    }
    if (!has_tiles) ImGui::EndDisabled();
    draw_clear_tiles_confirmation_popup();

    ImGui::Text("Tiles: %zu", current_world->get_chunks().size());
    if (heightmaps_dirty) {
        ImGui::TextDisabled("Unsaved height edits");
    }
}

void WorldEditor::draw_center_panel() {
    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    draw_viewport(viewport_size.x, viewport_size.y);
}

void WorldEditor::draw_viewport(float viewport_w, float viewport_h) {
    if (viewport_w <= 0.0f || viewport_h <= 0.0f) return;

    u32 need_w = std::max(1u, (u32)viewport_w);
    u32 need_h = std::max(1u, (u32)viewport_h);
    if (need_w != screen_width || need_h != screen_height) {
        screen_width = need_w;
        screen_height = need_h;
        screen_texture.create(TextureType::TEXTURE_2D, screen_width,
                              screen_height, PixelFormat::RGBA, nullptr);
        screen_depth.create(TextureType::TEXTURE_2D, screen_width,
                            screen_height, PixelFormat::D32, nullptr);
        picking_texture.create(TextureType::TEXTURE_2D, screen_width,
                               screen_height, PixelFormat::RGBA16I, nullptr);
        if (renderer != nullptr) {
            renderer->rebind_textures(screen_texture, screen_depth,
                                      ref_cast<Texture>(picking_texture));
        }
    }

    ImVec2 origin = ImGui::GetCursorScreenPos();
    if (current_world == nullptr) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            origin, ImVec2(origin.x + viewport_w, origin.y + viewport_h),
            IM_COL32(20, 22, 26, 255));
        const char *message = "No world loaded";
        ImVec2 text_size = ImGui::CalcTextSize(message);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(origin.x + (viewport_w - text_size.x) * 0.5f,
                   origin.y + (viewport_h - text_size.y) * 0.5f),
            IM_COL32(90, 96, 108, 255), message);
        ImGui::Dummy(ImVec2(viewport_w, viewport_h));
        return;
    }

    auto &chunks = current_world->get_chunks();
    if (chunks.empty()) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            origin, ImVec2(origin.x + viewport_w, origin.y + viewport_h),
            IM_COL32(20, 22, 26, 255));
        const char *message = "No terrain tiles";
        ImVec2 text_size = ImGui::CalcTextSize(message);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(origin.x + (viewport_w - text_size.x) * 0.5f,
                   origin.y + (viewport_h - text_size.y) * 0.5f),
            IM_COL32(90, 96, 108, 255), message);
        ImGui::Dummy(ImVec2(viewport_w, viewport_h));
        if (active_mode == WorldEditorMode::World) {
            add_chunk_from_empty_viewport_click(origin, viewport_w, viewport_h);
        }
        return;
    }

    ImGui::Image((ImTextureID)(u64)screen_texture->get_handle(),
                 ImVec2(viewport_w, viewport_h), ImVec2(0, 1), ImVec2(1, 0));

    ImVec2 mouse = ImGui::GetMousePos();
    last_image_valid =
        mouse.x >= origin.x && mouse.y >= origin.y &&
        mouse.x < origin.x + viewport_w && mouse.y < origin.y + viewport_h;
    if (last_image_valid) {
        last_image_x = (i32)(mouse.x - origin.x);
        last_image_y = (i32)(mouse.y - origin.y);
    }

    if (active_mode == WorldEditorMode::Terrain) {
        edit_terrain_viewport(origin, viewport_w, viewport_h);
    } else {
        add_chunk_from_empty_viewport_click(origin, viewport_w, viewport_h);
    }

    char overlay[96] = {};
    if (last_image_valid) {
        std::snprintf(overlay, sizeof(overlay), "Image: %d, %d",
                      last_image_x, last_image_y);
    } else {
        std::snprintf(overlay, sizeof(overlay), "Image: -");
    }
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 8.0f), IM_COL32(220, 225, 235, 255),
        overlay);

    if (last_pick_valid && active_mode == WorldEditorMode::Terrain) {
        std::snprintf(overlay, sizeof(overlay), "Terrain: %d, %d",
                      last_pick_x, last_pick_y);
    } else {
        std::snprintf(overlay, sizeof(overlay), "Terrain tiles: %zu",
                      chunks.size());
    }
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 24.0f), IM_COL32(220, 225, 235, 255),
        overlay);
}

void WorldEditor::edit_terrain_viewport(ImVec2 viewport_origin,
                                        float viewport_w, float viewport_h) {
    i32 world_x = 0;
    i32 world_y = 0;
    last_pick_valid =
        sample_terrain_pick(viewport_origin, viewport_w, viewport_h, world_x,
                            world_y);
    if (!last_pick_valid) return;

    last_pick_x = world_x;
    last_pick_y = world_y;

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 mouse = ImGui::GetMousePos();
    draw_list->AddCircle(mouse, brush_radius, IM_COL32(110, 180, 255, 220), 32,
                         1.5f);
    draw_list->AddCircle(mouse, brush_radius * 0.35f,
                         IM_COL32(110, 180, 255, 90), 24, 1.0f);

    if (terrain_tool == WorldTerrainTool::Pick) return;
    if (!ImGui::IsItemHovered()) return;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        apply_terrain_brush(world_x, world_y);
    }
}

void WorldEditor::draw_right_panel() {
    if (current_world == nullptr) {
        ImGui::TextDisabled("No world loaded.");
        return;
    }

    if (active_mode == WorldEditorMode::Terrain) {
        draw_terrain_panel();
    } else {
        draw_world_panel();
    }
}

void WorldEditor::draw_world_panel() {
    world_editor_section("World");

    char name_buffer[256] = {};
    const KString &world_name = current_world->get_name();
    if (!world_name.is_empty()) {
        std::snprintf(name_buffer, sizeof(name_buffer), "%s",
                      world_name.data());
    }
    if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
        current_world->set_name(name_buffer);
    }

    ImGui::Spacing();
    ImGui::Text("Terrain tiles: %zu", current_world->get_chunks().size());
    ImGui::Text("Height edits: %s", heightmaps_dirty ? "dirty" : "saved");

    world_editor_section("Directional Light");
    EditorDirectionalLight &light = current_world->get_directional_light();
    ImGui::Checkbox("Enabled", &light.enabled);
    draw_vec3_field("Direction", light.direction);
    draw_vec3_field("Diffuse", light.diffuse);
    draw_vec3_field("Specular", light.specular);
}

void WorldEditor::draw_terrain_panel() {
    world_editor_section("Terrain Tool");

    static const struct {
            const char *label;
            WorldTerrainTool tool;
    } tools[] = {{"Raise", WorldTerrainTool::Raise},
                 {"Lower", WorldTerrainTool::Lower},
                 {"Smooth", WorldTerrainTool::Smooth},
                 {"Flatten", WorldTerrainTool::Flatten},
                 {"Pick", WorldTerrainTool::Pick}};

    for (const auto &tool : tools) {
        bool selected = terrain_tool == tool.tool;
        if (ImGui::Selectable(tool.label, selected)) {
            terrain_tool = tool.tool;
        }
    }

    ImGui::Spacing();
    ImGui::Text("Active: %s", terrain_tool_name(terrain_tool));
    ImGui::SliderFloat("Radius", &brush_radius, 1.0f, 96.0f, "%.0f");
    ImGui::SliderFloat("Strength", &brush_strength, 0.01f, 1.0f, "%.2f");
    if (terrain_tool == WorldTerrainTool::Flatten) {
        ImGui::SliderFloat("Height", &flatten_height, 0.0f, 255.0f, "%.0f");
    }

    world_editor_section("Pick");
    if (last_image_valid) {
        ImGui::Text("Image: %d, %d", last_image_x, last_image_y);
    } else {
        ImGui::TextDisabled("Image: -");
    }
    if (last_pick_valid) {
        ImGui::Text("World: %d, %d", last_pick_x, last_pick_y);
        if (find_chunk_index_at_world(last_pick_x, last_pick_y) < 0) {
            ImGui::TextDisabled("No terrain tile");
        }
    } else {
        ImGui::TextDisabled("No terrain hit");
    }

    if (heightmaps_dirty &&
        ImGui::Button("Save Heightmaps", ImVec2(-1, 0))) {
        save_dirty_heightmaps();
        save_current_world();
    }
}

static bool valid_editor_heightmap(Ref<Image> heightmap) {
    return !heightmap.is_null() &&
           heightmap->get_width() >= WORLD_EDITOR_HEIGHTMAP_SIZE &&
           heightmap->get_height() >= WORLD_EDITOR_HEIGHTMAP_SIZE;
}

static void copy_editor_height_pixel(Ref<Image> src, u32 src_x, u32 src_y,
                                     Ref<Image> dst, u32 dst_x, u32 dst_y) {
    u8 *src_pixel = src->pixel(src_x, src_y);
    u8 *dst_pixel = dst->pixel(dst_x, dst_y);
    dst_pixel[0] = src_pixel[0];
    dst_pixel[1] = src_pixel[1];
}

static void copy_editor_heightmap_column(Ref<Image> src, u32 src_x,
                                         Ref<Image> dst, u32 dst_x) {
    if (!valid_editor_heightmap(src) || !valid_editor_heightmap(dst)) return;
    for (u32 y = 0; y < WORLD_EDITOR_HEIGHTMAP_SIZE; y++) {
        copy_editor_height_pixel(src, src_x, y, dst, dst_x, y);
    }
}

static void copy_editor_heightmap_row(Ref<Image> src, u32 src_y,
                                      Ref<Image> dst, u32 dst_y) {
    if (!valid_editor_heightmap(src) || !valid_editor_heightmap(dst)) return;
    for (u32 x = 0; x < WORLD_EDITOR_HEIGHTMAP_SIZE; x++) {
        copy_editor_height_pixel(src, x, src_y, dst, x, dst_y);
    }
}

void WorldEditor::sync_heightmap_seams(std::set<u32> &touched_chunks) {
    if (current_world == nullptr) return;

    std::set<u32> source_chunks = touched_chunks;
    for (u32 chunk_idx : source_chunks) {
        if (chunk_idx >= current_world->chunks.size() ||
            chunk_idx >= current_world->heightmaps.size()) {
            continue;
        }

        const EditorChunk &chunk = current_world->chunks[chunk_idx];
        Ref<Image> src = current_world->heightmaps[chunk_idx];
        if (!valid_editor_heightmap(src)) continue;

        i32 left_idx = find_chunk_index_at_chunk(chunk.x - 1, chunk.y);
        if (left_idx >= 0 &&
            (u32)left_idx < current_world->heightmaps.size()) {
            Ref<Image> dst = current_world->heightmaps[left_idx];
            copy_editor_heightmap_column(src, 0, dst,
                                         WORLD_EDITOR_HEIGHTMAP_LAST);
            touched_chunks.insert((u32)left_idx);
        }

        i32 bottom_idx = find_chunk_index_at_chunk(chunk.x, chunk.y - 1);
        if (bottom_idx >= 0 &&
            (u32)bottom_idx < current_world->heightmaps.size()) {
            Ref<Image> dst = current_world->heightmaps[bottom_idx];
            copy_editor_heightmap_row(src, 0, dst,
                                      WORLD_EDITOR_HEIGHTMAP_LAST);
            touched_chunks.insert((u32)bottom_idx);
        }
    }
}

void WorldEditor::apply_terrain_brush(i32 world_x, i32 world_y) {
    if (current_world == nullptr || current_world->terrain.is_null()) return;

    i32 radius = std::max(1, (i32)brush_radius);
    std::set<u32> touched_chunks;

    auto read_height = [&](i32 wx, i32 wy, u8 &height) -> bool {
        u32 chunk_idx = 0;
        u32 px = 0;
        u32 py = 0;
        if (!world_to_heightmap_pixel(wx, wy, chunk_idx, px, py)) return false;
        if (chunk_idx >= current_world->heightmaps.size() ||
            current_world->heightmaps[chunk_idx].is_null()) {
            return false;
        }
        height = current_world->heightmaps[chunk_idx]->pixel(px, py)[1];
        return true;
    };

    auto write_height = [&](i32 wx, i32 wy, u8 height) -> bool {
        u32 chunk_idx = 0;
        u32 px = 0;
        u32 py = 0;
        if (!world_to_heightmap_pixel(wx, wy, chunk_idx, px, py)) return false;
        if (chunk_idx >= current_world->heightmaps.size() ||
            current_world->heightmaps[chunk_idx].is_null()) {
            return false;
        }
        current_world->heightmaps[chunk_idx]->pixel(px, py)[1] = height;
        touched_chunks.insert(chunk_idx);
        return true;
    };

    for (i32 dy = -radius; dy <= radius; dy++) {
        for (i32 dx = -radius; dx <= radius; dx++) {
            f32 dist = std::sqrt((f32)(dx * dx + dy * dy));
            if (dist > brush_radius) continue;

            f32 falloff = 1.0f - dist / std::max(1.0f, brush_radius);
            f32 amount = brush_strength * falloff;
            i32 sample_x = world_x + dx;
            i32 sample_y = world_y + dy;

            u8 current = 0;
            if (!read_height(sample_x, sample_y, current)) continue;

            i32 next = current;
            switch (terrain_tool) {
                case WorldTerrainTool::Raise:
                    next = current + (i32)std::round(255.0f * amount);
                    break;
                case WorldTerrainTool::Lower:
                    next = current - (i32)std::round(255.0f * amount);
                    break;
                case WorldTerrainTool::Flatten:
                    next = current +
                           (i32)std::round((flatten_height - current) * amount);
                    break;
                case WorldTerrainTool::Smooth: {
                    i32 sum = 0;
                    i32 count = 0;
                    for (i32 sy = -1; sy <= 1; sy++) {
                        for (i32 sx = -1; sx <= 1; sx++) {
                            u8 neighbor = 0;
                            if (read_height(sample_x + sx, sample_y + sy,
                                            neighbor)) {
                                sum += neighbor;
                                count++;
                            }
                        }
                    }
                    if (count > 0) {
                        f32 target = (f32)sum / (f32)count;
                        next = current +
                               (i32)std::round((target - current) * amount);
                    }
                    break;
                }
                case WorldTerrainTool::Pick:
                    continue;
            }

            next = std::clamp(next, 0, 255);
            write_height(sample_x, sample_y, (u8)next);
        }
    }

    sync_heightmap_seams(touched_chunks);

    for (u32 chunk_idx : touched_chunks) {
        current_world->terrain->update_chunk_heightmap(
            chunk_idx, current_world->heightmaps[chunk_idx]);
        dirty_heightmaps.insert(chunk_idx);
    }
    if (!touched_chunks.empty()) {
        heightmaps_dirty = true;
    }
}

void WorldEditor::update() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float left_w = std::min(250.0f, avail.x * 0.28f);
    const float right_w = std::min(340.0f, avail.x * 0.35f);
    const float gap = 4.0f;
    const float center_w =
        std::max(120.0f, avail.x - left_w - right_w - gap * 2.0f);
    const float panel_h = avail.y;

    ImGui::BeginChild("##we_left", ImVec2(left_w, panel_h), false);
    draw_left_panel();
    ImGui::EndChild();

    ImGui::SameLine(0, gap);

    ImGui::BeginChild("##we_center", ImVec2(center_w, panel_h), false);
    draw_center_panel();
    ImGui::EndChild();

    ImGui::SameLine(0, gap);

    ImGui::BeginChild("##we_right", ImVec2(right_w, panel_h), false);
    draw_right_panel();
    ImGui::EndChild();
}

}  // namespace Seed
