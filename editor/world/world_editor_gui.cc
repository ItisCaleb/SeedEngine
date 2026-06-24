#include "world_editor.h"
#include "editor/world/editor_world.h"
#include <algorithm>
#include <cstdio>
#include <imgui.h>
#include "core/resource/resource_loader.h"
#include "editor/gui/editor_ui.h"

namespace Seed {

#define WORLD_EDITOR_TERRAIN_PREVIEW_SIZE (64.0f)

void WorldEditor::draw_terrain_palette_tooltip(
    UUID uuid, const EditorUI::TexturePreview *preview) {
    ImGui::BeginTooltip();
    std::string label = terrain_texture_label(uuid);
    ImGui::TextUnformatted(label.c_str());
    if (preview != nullptr && preview->width != 0 && preview->height != 0) {
        ImGui::TextDisabled("%u x %u", preview->width, preview->height);
    }
    if (!uuid.is_null()) {
        ResourceEntry *entry =
            ResourceLoader::get_instance()->get_entries().get_entry(uuid);
        if (entry != nullptr) {
            KStr path = entry->path.to_str();
            ImGui::TextDisabled("%.*s", (i32)path.length(), path.data());
        }
    }
    if (preview != nullptr && preview->upload_failed) {
        ImGui::TextDisabled("Array upload skipped");
    }
    ImGui::EndTooltip();
}

void WorldEditor::draw_terrain_palette_map(u32 index, TerrainPaletteMap map) {
    EditorUI::ScopedID map_id((i32)map);
    EditorUI::ScopedGroup group;

    UUID uuid = terrain_palette_uuid(index, map);
    const EditorUI::TexturePreview *preview =
        terrain_palette_preview(index, map);

    ImGui::TextUnformatted(terrain_palette_map_label(map));
    ImGui::InvisibleButton("##preview",
                           ImVec2(WORLD_EDITOR_TERRAIN_PREVIEW_SIZE,
                                  WORLD_EDITOR_TERRAIN_PREVIEW_SIZE));
    ImVec2 preview_min = ImGui::GetItemRectMin();
    ImVec2 preview_max = ImGui::GetItemRectMax();
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
        selected_terrain_palette_slot = (i32)index;
    }

    bool selected = selected_terrain_palette_slot == (i32)index;
    EditorUI::draw_texture_preview(preview, preview_min, preview_max, selected);

    bool hovered = ImGui::IsItemHovered();
    accept_terrain_palette_drop(index, map);
    if (hovered) {
        draw_terrain_palette_tooltip(uuid, preview);
    }

    std::string label = terrain_texture_label(uuid);
    ImGui::TextWrapped("%s", label.c_str());
    if (preview != nullptr && preview->width != 0 && preview->height != 0) {
        ImGui::TextDisabled("%u x %u", preview->width, preview->height);
    } else if (uuid.is_null()) {
        ImGui::TextDisabled("Empty");
    } else {
        ImGui::TextDisabled("Preview unavailable");
    }
}

void WorldEditor::draw_clear_tiles_confirmation_popup() {
    if (ImGui::BeginPopupModal("Clear Terrain Tiles?", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped(
            "This removes every terrain tile from the current world.");
        ImGui::TextDisabled(
            "Heightmap asset references are removed from the world.");
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

void WorldEditor::draw_left_panel() {
    EditorUI::section("World");

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

    EditorUI::section("Mode");

    if (ImGui::Selectable("World", active_mode == WorldEditorMode::World)) {
        active_mode = WorldEditorMode::World;
    }
    if (ImGui::Selectable("Terrain", active_mode == WorldEditorMode::Terrain)) {
        active_mode = WorldEditorMode::Terrain;
    }

    EditorUI::section("Terrain");

    if (current_world == nullptr) {
        ImGui::TextDisabled("No world loaded.");
        return;
    }

    if (ImGui::Button("Add Terrain Tile", ImVec2(-1, 0))) {
        add_chunk();
    }

    bool has_tiles = !current_world->get_chunks().empty();
    {
        EditorUI::DisabledScope disabled(!has_tiles);
        if (ImGui::Button("Clear Terrain Tiles", ImVec2(-1, 0))) {
            ImGui::OpenPopup("Clear Terrain Tiles?");
        }
    }
    draw_clear_tiles_confirmation_popup();

    ImGui::Text("Tiles: %zu", current_world->get_chunks().size());
    if (heightmaps_dirty || controlmaps_dirty) {
        ImGui::TextDisabled("Unsaved terrain map edits");
    }
}

void WorldEditor::draw_center_panel() {
    ImVec2 viewport_size = ImGui::GetContentRegionAvail();
    draw_viewport(viewport_size.x, viewport_size.y);
}

void WorldEditor::draw_empty_viewport(ImVec2 origin, f32 viewport_w,
                                      f32 viewport_h, const char *message) {
    EditorUI::draw_empty_panel(
        ImGui::GetWindowDrawList(), origin,
        ImVec2(origin.x + viewport_w, origin.y + viewport_h), message);
    ImGui::Dummy(ImVec2(viewport_w, viewport_h));
}
void WorldEditor::draw_viewport(f32 viewport_w, f32 viewport_h) {
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
        draw_empty_viewport(origin, viewport_w, viewport_h, "No world loaded");
        return;
    }

    std::vector<ChunkSetting> &chunks = current_world->get_chunks();
    if (chunks.empty()) {
        draw_empty_viewport(origin, viewport_w, viewport_h, "No terrain tiles");
        if (active_mode == WorldEditorMode::World) {
            add_chunk_from_empty_viewport_click(origin, viewport_w, viewport_h);
        }
        return;
    }

    ImGui::Image((ImTextureID)(u64)screen_texture->get_handle(),
                 ImVec2(viewport_w, viewport_h), ImVec2(0, 1), ImVec2(1, 0));

    ImVec2 mouse = ImGui::GetMousePos();
    last_image_valid = mouse.x >= origin.x && mouse.y >= origin.y &&
                       mouse.x < origin.x + viewport_w &&
                       mouse.y < origin.y + viewport_h;
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
        std::snprintf(overlay, sizeof(overlay), "Image: %d, %d", last_image_x,
                      last_image_y);
    } else {
        std::snprintf(overlay, sizeof(overlay), "Image: -");
    }
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 8.0f), IM_COL32(220, 225, 235, 255),
        overlay);

    if (last_pick_valid && active_mode == WorldEditorMode::Terrain) {
        std::snprintf(overlay, sizeof(overlay), "Terrain: %d, %d", last_pick_x,
                      last_pick_y);
    } else {
        std::snprintf(overlay, sizeof(overlay), "Terrain tiles: %zu",
                      chunks.size());
    }
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 24.0f), IM_COL32(220, 225, 235, 255),
        overlay);
}

void WorldEditor::edit_terrain_viewport(ImVec2 viewport_origin, f32 viewport_w,
                                        f32 viewport_h) {
    i32 world_x = 0;
    i32 world_y = 0;
    last_pick_valid = sample_terrain_pick(viewport_origin, viewport_w,
                                          viewport_h, world_x, world_y);
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
    EditorUI::section("World");

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
    ImGui::Text("Terrain maps: %s",
                (heightmaps_dirty || controlmaps_dirty) ? "dirty" : "saved");

    EditorUI::section("Directional Light");
    DirectionalLightSetting &light = current_world->get_directional_light();
    bool changed = EditorUI::draw_vec3("Direction", light.direction);
    changed |= EditorUI::draw_vec3("Diffuse", light.diffuse);
    changed |= EditorUI::draw_vec3("Specular", light.specular);
    if (changed) {
        current_world->apply_directional_light_to_runtime();
    }
}

void WorldEditor::draw_terrain_palette() {
    EditorUI::section("Terrain Palette");
    if (current_world == nullptr) return;

    std::vector<UUID> &textures = current_world->get_terrain_textures();
    if (selected_terrain_palette_slot >= (i32)textures.size()) {
        selected_terrain_palette_slot = -1;
    }

    if (ImGui::Button("Add Terrain Layer", ImVec2(-1, 0))) {
        current_world->add_terrain_texture(UUID{});
        selected_terrain_palette_slot = (i32)textures.size() - 1;
        save_current_world();
        status_text = "Terrain layer added.";
    }

    if (textures.empty()) {
        ImGui::TextDisabled("No terrain layers.");
        return;
    }

    for (u32 i = 0; i < textures.size(); i++) {
        EditorUI::ScopedID row_id((i32)i);

        ImGui::Text("Layer %u", i);
        ImGui::SameLine();
        bool remove = false;
        if (ImGui::SmallButton("Remove")) {
            remove = true;
        }

        draw_terrain_palette_map(i, TerrainPaletteMap::Albedo);
        ImGui::SameLine();
        draw_terrain_palette_map(i, TerrainPaletteMap::Normal);

        if (remove) {
            remove_terrain_palette_slot(i);
            break;
        }

        ImGui::Separator();
    }
}

void WorldEditor::draw_terrain_panel() {
    EditorUI::section("Terrain Tool");

    if (ImGui::Selectable("Raise", terrain_tool == WorldTerrainTool::Raise)) {
        terrain_tool = WorldTerrainTool::Raise;
    }
    if (ImGui::Selectable("Lower", terrain_tool == WorldTerrainTool::Lower)) {
        terrain_tool = WorldTerrainTool::Lower;
    }
    if (ImGui::Selectable("Smooth", terrain_tool == WorldTerrainTool::Smooth)) {
        terrain_tool = WorldTerrainTool::Smooth;
    }
    if (ImGui::Selectable("Flatten",
                          terrain_tool == WorldTerrainTool::Flatten)) {
        terrain_tool = WorldTerrainTool::Flatten;
    }
    if (ImGui::Selectable("Pick", terrain_tool == WorldTerrainTool::Pick)) {
        terrain_tool = WorldTerrainTool::Pick;
    }
    if (ImGui::Selectable("Splat", terrain_tool == WorldTerrainTool::Splat)) {
        terrain_tool = WorldTerrainTool::Splat;
    }

    ImGui::Spacing();
    ImGui::Text("Active: %s", terrain_tool_name(terrain_tool));
    ImGui::SliderFloat("Radius", &brush_radius, 1.0f, 96.0f, "%.0f");
    ImGui::SliderFloat("Strength", &brush_strength, 0.01f, 1.0f, "%.2f");
    if (terrain_tool == WorldTerrainTool::Flatten) {
        ImGui::SliderFloat("Height", &flatten_height, 0.0f, 255.0f, "%.0f");
    }

    draw_terrain_palette();

    EditorUI::section("Pick");
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

    if ((heightmaps_dirty || controlmaps_dirty) &&
        ImGui::Button("Save Terrain Maps", ImVec2(-1, 0))) {
        save_dirty_heightmaps();
        save_dirty_controlmaps();
        save_current_world();
    }
}

void WorldEditor::update() {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const f32 left_w = std::min(250.0f, avail.x * 0.28f);
    const f32 right_w = std::min(340.0f, avail.x * 0.35f);
    const f32 gap = 4.0f;
    const f32 center_w =
        std::max(120.0f, avail.x - left_w - right_w - gap * 2.0f);
    const f32 panel_h = avail.y;

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
