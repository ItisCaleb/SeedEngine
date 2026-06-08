#include "world_editor.h"
#include <algorithm>
#include <cstdio>
#include <imgui.h>
#include <nfd.h>
#include "core/engine.h"
#include "core/io/file.h"
#include "core/misc/uuid.h"
#include "core/project.h"
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

static const char *safe_path_text(const Path &path) {
    KStr text = path.to_str();
    return text.is_empty() ? "" : text.data();
}

void WorldEditor::init() {
    default_heightmap.create(PixelFormat::RG, 257, 257);
    default_heightmap->fill(Color{0, 0}, 257, 257);
    screen_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                          PixelFormat::RGBA, nullptr);
    screen_depth.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                        PixelFormat::D32, nullptr);
    picking_texture.create(TextureType::TEXTURE_2D, screen_width, screen_height,
                           PixelFormat::RGBA16I, nullptr);

    renderer = new WorldRenderer(screen_texture, screen_depth, picking_texture);
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

void WorldEditor::validate_selected_chunk() {
    if (current_world == nullptr || current_world->get_chunks().empty()) {
        selected_chunk = -1;
        return;
    }

    i32 chunk_count = (i32)current_world->get_chunks().size();
    if (selected_chunk < 0) selected_chunk = 0;
    if (selected_chunk >= chunk_count) selected_chunk = chunk_count - 1;
}

void WorldEditor::mark_preview_terrain_dirty() {
    preview_terrain_dirty = true;
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
            selected_chunk = -1;
            return false;
        }

        standalone_config = ResourceConfiguration(file->read_json());
        current_world = std::make_unique<EditorWorld>(&standalone_config);
        current_world_path = path;
    }

    validate_selected_chunk();
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
    auto &chunks = current_world->get_chunks();

    EditorChunk chunk;
    chunk.x = (u32)chunks.size();
    chunk.y = 0;
    chunks.push_back(chunk);
    selected_chunk = (i32)chunks.size() - 1;
    current_world->terrain->add_chunk(chunk.x, chunk.y, default_heightmap);
}

void WorldEditor::remove_selected_chunk() {
    if (current_world == nullptr) return;
    auto &chunks = current_world->get_chunks();
    if (selected_chunk < 0 || selected_chunk >= (i32)chunks.size()) return;

    chunks.erase(chunks.begin() + selected_chunk);
    validate_selected_chunk();
}

void WorldEditor::draw_uuid_field(const char *label, UUID &uuid) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    KString uuid_text = uuid.to_string();
    ImGui::TextUnformatted(uuid_text.data());
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload("UUID")) {
            uuid = *(UUID *)payload->Data;
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::PopID();
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
        ImGui::TextWrapped("%s", safe_path_text(current_world_path));
    }

    if (!status_text.empty()) {
        ImGui::Spacing();
        ImGui::TextWrapped("%s", status_text.c_str());
    }

    world_editor_section("Chunks");

    if (current_world == nullptr) {
        ImGui::TextDisabled("No world loaded.");
        return;
    }

    if (ImGui::Button("Add Chunk", ImVec2(-1, 0))) {
        add_chunk();
    }

    auto &chunks = current_world->get_chunks();
    for (i32 i = 0; i < (i32)chunks.size(); i++) {
        char label[64] = {};
        std::snprintf(label, sizeof(label), "Chunk (%u, %u)", chunks[i].x,
                      chunks[i].y);
        if (ImGui::Selectable(label, selected_chunk == i)) {
            selected_chunk = i;
        }
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
                                      picking_texture);
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
        const char *message = "No chunks";
        ImVec2 text_size = ImGui::CalcTextSize(message);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(origin.x + (viewport_w - text_size.x) * 0.5f,
                   origin.y + (viewport_h - text_size.y) * 0.5f),
            IM_COL32(90, 96, 108, 255), message);
        ImGui::Dummy(ImVec2(viewport_w, viewport_h));
        return;
    }

    ImGui::Image((ImTextureID)(u64)screen_texture->get_handle(),
                 ImVec2(viewport_w, viewport_h), ImVec2(0, 1), ImVec2(1, 0));

    char overlay[64] = {};
    std::snprintf(overlay, sizeof(overlay), "Chunks: %zu", chunks.size());
    ImGui::GetWindowDrawList()->AddText(
        ImVec2(origin.x + 8.0f, origin.y + 8.0f), IM_COL32(220, 225, 235, 255),
        overlay);
}

void WorldEditor::draw_point_lights(EditorChunk &chunk) {
    if (!ImGui::CollapsingHeader("Point Lights",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    if (ImGui::Button("Add Point Light")) {
        EditorPointLight light;
        chunk.lights.push_back(light);
    }

    for (i32 i = 0; i < (i32)chunk.lights.size();) {
        ImGui::PushID(i);
        char label[64] = {};
        std::snprintf(label, sizeof(label), "Light %d", i);
        bool open = ImGui::TreeNode(label);
        ImGui::SameLine();
        bool remove = ImGui::SmallButton("Remove");
        if (open) {
            EditorPointLight &light = chunk.lights[i];
            draw_vec3_field("Position", light.position);
            draw_vec3_field("Diffuse", light.diffuse);
            draw_vec3_field("Specular", light.specular);
            ImGui::TreePop();
        }
        ImGui::PopID();

        if (remove) {
            chunk.lights.erase(chunk.lights.begin() + i);
            continue;
        }
        i++;
    }
}

void WorldEditor::draw_static_objects(EditorChunk &chunk) {
    if (!ImGui::CollapsingHeader("Static Objects",
                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    if (ImGui::Button("Add Static Object")) {
        EditorStaticObject object;
        object.name = "Static Object";
        chunk.static_objects.push_back(object);
    }

    for (i32 i = 0; i < (i32)chunk.static_objects.size();) {
        ImGui::PushID(i);
        EditorStaticObject &object = chunk.static_objects[i];
        char label[128] = {};
        if (!object.name.is_empty()) {
            std::snprintf(label, sizeof(label), "%s", object.name.data());
        } else {
            std::snprintf(label, sizeof(label), "Object %d", i);
        }

        bool open = ImGui::TreeNode(label);
        ImGui::SameLine();
        bool remove = ImGui::SmallButton("Remove");
        if (open) {
            char name_buffer[256] = {};
            if (!object.name.is_empty()) {
                std::snprintf(name_buffer, sizeof(name_buffer), "%s",
                              object.name.data());
            }
            if (ImGui::InputText("Name", name_buffer, sizeof(name_buffer))) {
                object.name = name_buffer;
            }

            ImGui::InputInt("X", &object.x);
            ImGui::InputInt("Y", &object.y);
            draw_uuid_field("Model", object.model);
            ImGui::TreePop();
        }
        ImGui::PopID();

        if (remove) {
            chunk.static_objects.erase(chunk.static_objects.begin() + i);
            continue;
        }
        i++;
    }
}

void WorldEditor::draw_right_panel() {
    validate_selected_chunk();

    if (current_world == nullptr) {
        ImGui::TextDisabled("No world loaded.");
        return;
    }

    if (selected_chunk < 0) {
        ImGui::TextDisabled("No chunk selected.");
        return;
    }

    auto &chunks = current_world->get_chunks();
    EditorChunk &chunk = chunks[selected_chunk];

    world_editor_section("Selected Chunk");

    int x = (int)chunk.x;
    int y = (int)chunk.y;
    if (ImGui::InputInt("X", &x)) {
        chunk.x = (u32)std::max(0, x);
    }
    if (ImGui::InputInt("Y", &y)) {
        chunk.y = (u32)std::max(0, y);
    }

    draw_uuid_field("Height Map", chunk.height_map);

    if (ImGui::Button("Remove Chunk", ImVec2(-1, 0))) {
        remove_selected_chunk();
        return;
    }

    ImGui::Spacing();
    draw_point_lights(chunk);
    draw_static_objects(chunk);
}

void WorldEditor::update() {
    validate_selected_chunk();

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
