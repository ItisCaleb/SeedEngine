#include "asset_browser.h"
#include <fmt/base.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <utility>
#include <vector>
#include "asset.h"
#include "core/container/kstring.h"
#include "core/input.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/resource/resource.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"
#include "editor/project/preprocessor.h"

namespace Seed {

// ── Classification ───────────────────────────────────────
AssetType AssetBrowser::classify(const Path &p) {
    if (p.is_directory()) return AssetType::Directory;
    KString extension = p.extension().string().to_lower();
    // lowercase
    KStr ext = extension;

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
        ext == ".exr" || ext == ".hdr")
        return AssetType::Texture;
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb")
        return AssetType::Mesh;
    if (ext == ".terrain" || ext == ".json")  // adjust to your format
        return AssetType::Terrain;
    if (ext == ".mat") return AssetType::Material;
    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3")
        return AssetType::Audio;
    if (ext == ".cpp" || ext == ".h" || ext == ".lua" || ext == ".py")
        return AssetType::Script;
    return AssetType::Unknown;
}

const char *AssetBrowser::asset_type_icon(AssetType t) {
    switch (t) {
        case AssetType::Directory:
            return "[D]";
        case AssetType::Texture:
            return "[T]";
        case AssetType::Mesh:
            return "[M]";
        case AssetType::Terrain:
            return "[TR]";
        case AssetType::Material:
            return "[MT]";
        case AssetType::Audio:
            return "[A]";
        case AssetType::Script:
            return "[S]";
        default:
            return "[?]";
    }
}

ImVec4 AssetBrowser::asset_type_color(AssetType t) {
    switch (t) {
        case AssetType::Directory:
            return ImVec4(0.9f, 0.75f, 0.3f, 1.f);
        case AssetType::Texture:
            return ImVec4(0.4f, 0.75f, 0.9f, 1.f);
        case AssetType::Mesh:
            return ImVec4(0.6f, 0.9f, 0.5f, 1.f);
        case AssetType::Terrain:
            return ImVec4(0.5f, 0.8f, 0.4f, 1.f);
        case AssetType::Material:
            return ImVec4(0.9f, 0.5f, 0.7f, 1.f);
        case AssetType::Audio:
            return ImVec4(0.7f, 0.5f, 0.9f, 1.f);
        case AssetType::Script:
            return ImVec4(0.9f, 0.6f, 0.3f, 1.f);
        default:
            return ImVec4(0.5f, 0.5f, 0.5f, 1.f);
    }
}

// ── Init / Navigate ──────────────────────────────────────
void AssetBrowser::init(KStr project_root) {
    root_dir = Dir::open(project_root);
    current_dir = Dir::open(project_root);
    navigate_to(project_root);
}

void AssetBrowser::navigate_to(KStr dir) {
    current_dir = Dir::open(dir);
    selected_idx = -1;
    renaming_idx = -1;
    needs_refresh = true;

    // Rebuild breadcrumbs: root → dir
    breadcrumbs.clear();
    Path p = dir;
    while (p != root_dir->get_path()) {
        breadcrumbs.insert(breadcrumbs.begin(), p);
        if (p == root_dir->get_path()) break;
        p = p.parent();
    }
}

void AssetBrowser::refresh() {
    entries.clear();
    if (!current_dir.is_valid()) return;

    // Dirs first, then files, both sorted alphabetically
    std::vector<Path> dirs, files;
    std::vector<Path> childrens = current_dir->list();
    for (auto &path : childrens) {
        if (path.is_directory())
            dirs.push_back(std::move(path));
        else
            files.push_back(std::move(path));
    }
    std::sort(dirs.begin(), dirs.end());
    std::sort(files.begin(), files.end());

    for (auto &d : dirs)
        entries.push_back(AssetEntry{
            .path = std::move(d),
            .type = AssetType::Directory,
            .is_dir = true,
        });
    for (auto &f : files) {
        AssetType type = classify(f);
        entries.push_back(AssetEntry{
            .path = std::move(f),
            .type = type,
            .is_dir = false,
        });
    }

    needs_refresh = false;
}

// ── Main update ──────────────────────────────────────────
void AssetBrowser::update() {
    if (needs_refresh) refresh();

    draw_toolbar();
    ImGui::Separator();
    draw_breadcrumb();
    ImGui::Separator();

    // Main content area
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("##ab_content", avail, false,
                      ImGuiWindowFlags_NoScrollbar);

    // Filter by search
    KStr filter = search_buf;

    // Build filtered index list
    std::vector<int> visible;
    for (int i = 0; i < (int)entries.size(); i++) {
        if (filter.is_empty()) {
            visible.push_back(i);
            continue;
        }
        KStr name = entries[i].path.filename();
        i32 find = name.find_first(filter);
        if (find != -1) visible.push_back(i);
    }
    if (view_mode == 0)
        draw_grid();  // passes visible internally via state — refactor if
                      // needed
    else
        draw_list();

    // Click on empty area → deselect
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        selected_idx = -1;
    /* drop target*/
    ImVec2 remaining = ImGui::GetContentRegionAvail();
    if (remaining.y > 0) {
        ImGui::InvisibleButton("##grid_drop", remaining);
        if (ImGui::BeginDragDropTarget()) {
            if (auto p = ImGui::AcceptDragDropPayload("EXTERNAL")) {
                KStr data(KStr((char *)p->Data, p->DataSize - 1));
                std::vector<KStr> files = data.split("\n");
                fmt::println("{}", files);

                for (auto file : files) {
                    if (file.is_empty()) continue;
                    gEditor->project()->add_to_project(
                        file, current_dir->get_path().directory());
                }
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::EndChild();
}

// ── Toolbar ──────────────────────────────────────────────
void AssetBrowser::draw_toolbar() {
    // Search box
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##ab_search", "Search...", search_buf,
                             sizeof(search_buf));

    ImGui::SameLine(0, 12);

    // View mode toggle
    if (ImGui::RadioButton("Grid", view_mode == 0)) view_mode = 0;
    ImGui::SameLine();
    if (ImGui::RadioButton("List", view_mode == 1)) view_mode = 1;

    ImGui::SameLine(0, 12);

    // Icon size (grid only)
    if (view_mode == 0) {
        ImGui::SetNextItemWidth(80);
        ImGui::SliderFloat("##iconsize", &icon_size, 48.f, 120.f, "%.0f");
        ImGui::SameLine();
    }

    // Refresh button
    if (ImGui::SmallButton("Refresh")) needs_refresh = true;

    ImGui::SameLine(0, 12);

    // "New folder" shortcut
    if (ImGui::SmallButton("+ Folder")) {
        Path new_dir = current_dir->get_path();
        new_dir.push("New Folder");
        int suffix = 0;
        while (Dir::exists(new_dir)) {
            new_dir.pop();
            new_dir.push("New Folder " + std::to_string(++suffix));
        }
        Dir::create_if_not_exists(new_dir);
        needs_refresh = true;
    }
}

// ── Breadcrumb ───────────────────────────────────────────
void AssetBrowser::draw_breadcrumb() {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
    for (int i = 0; i < (int)breadcrumbs.size(); i++) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled(">");
            ImGui::SameLine();
        }
        KStr label =
            i == 0
                ? root_dir->get_path().filename()  // show project name for root
                : breadcrumbs[i].filename();

        bool is_current = (breadcrumbs[i] == current_dir->get_path());
        if (is_current) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.f));
            ImGui::TextUnformatted(label.data());
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 0.9f, 1.f));
            if (ImGui::SmallButton(label.data()))
                navigate_to(breadcrumbs[i].to_str());
            ImGui::PopStyleColor();
        }
    }
    ImGui::PopStyleVar();
}

UUID AssetBrowser::get_asset_uuid(AssetEntry &entry) {
    /* TODO: this is just a workaround */
    Path _p = "assets";
    _p.push(entry.path);

    if (entry.type == AssetType::Mesh) {
        PreprocessEntry *pentry = gEditor->preprocessor.get_entry_from_path(_p);
        if (pentry == nullptr) return UUID();
        return pentry->target_uuid;
    } else {
        UUID uuid = ResourceLoader::get_instance()->get_entries().get_uuid(_p);
        return uuid;
    }
}

Inspectable *AssetBrowser::create_inspectable(AssetEntry &entry) {
    if (entry.type == AssetType::Mesh) {
        ResourceEntry *rentry =
            ResourceLoader::get_instance()->get_entries().get_entry(
                get_asset_uuid(entry));
        if (rentry == nullptr) return nullptr;
        return new ModelInspector(rentry->config);
    }
    return nullptr;
}

// ── Grid view ────────────────────────────────────────────
void AssetBrowser::draw_grid() {
    KStr filter = search_buf;

    float cell_w = icon_size + 16.f;
    float cell_h = icon_size + 32.f;
    float avail_w = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, (int)(avail_w / cell_w));

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    int col = 0;
    for (int i = 0; i < (int)entries.size(); i++) {
        auto &e = entries[i];

        // Filter
        if (!filter.is_empty()) {
            KStr name = e.path.filename().string();
            if (name.find_first(filter) != -1) continue;
        }

        if (col > 0 && col % cols != 0) ImGui::SameLine(0, 8);

        ImGui::PushID(i);

        bool is_selected = (i == selected_idx);
        ImVec2 cell_size = ImVec2(cell_w, cell_h);
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        // Selection highlight
        if (is_selected) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursor, ImVec2(cursor.x + cell_size.x, cursor.y + cell_size.y),
                IM_COL32(60, 100, 160, 180), 4.f);
        }

        // Invisible selectable covering the whole cell
        if (ImGui::InvisibleButton("##cell", cell_size)) {
            if (e.is_dir) {
                navigate_to(e.path.to_str());
            } else {
                selected_idx = i;
                gEditor->set_current_inspect(create_inspectable(e));
            }
        }

        // Double-click to open
        if (ImGui::IsItemHovered() &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            if (e.is_dir) navigate_to(e.path.to_str());
            // else: emit open event to editor
        }

        // Hover highlight
        if (ImGui::IsItemHovered() && !is_selected) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursor, ImVec2(cursor.x + cell_size.x, cursor.y + cell_size.y),
                IM_COL32(80, 80, 80, 120), 4.f);
        }

        // Right-click context menu
        if (ImGui::BeginPopupContextItem("##ctx")) {
            selected_idx = i;
            draw_entry_context_menu(i);
            ImGui::EndPopup();
        }

        // Drag source — payload is the absolute path string
        if (!e.is_dir &&
            ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            UUID uuid = get_asset_uuid(e);
            if (!uuid.is_null()) {
                ImGui::SetDragDropPayload("UUID", &uuid, sizeof(uuid));
                ImGui::TextUnformatted(uuid.to_string().c_str());
            }

            ImGui::EndDragDropSource();
        }

        // Draw icon and label on top of the selectable
        ImGui::SetCursorScreenPos(cursor);
        ImGui::BeginGroup();

        // Icon area
        ImVec4 col4 = asset_type_color(e.type);
        ImVec2 icon_pos = ImGui::GetCursorScreenPos();
        float icon_pad = 8.f;
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(icon_pos.x + icon_pad, icon_pos.y),
            ImVec2(icon_pos.x + cell_size.x - icon_pad, icon_pos.y + icon_size),
            ImGui::ColorConvertFloat4ToU32(
                ImVec4(col4.x * 0.3f, col4.y * 0.3f, col4.z * 0.3f, 1.f)),
            4.f);

        // If thumbnail is available draw it, otherwise draw type label
        if (e.thumbnail_handle != 0) {
            ImGui::SetCursorScreenPos(
                ImVec2(icon_pos.x + icon_pad, icon_pos.y));
            ImGui::Image((ImTextureID)e.thumbnail_handle,
                         ImVec2(cell_size.x - icon_pad * 2, icon_size));
        } else {
            const char *type_label = asset_type_icon(e.type);
            ImVec2 tl_size = ImGui::CalcTextSize(type_label);
            ImGui::GetWindowDrawList()->AddText(
                ImVec2(icon_pos.x + (cell_size.x - tl_size.x) * 0.5f,
                       icon_pos.y + (icon_size - tl_size.y) * 0.5f),
                ImGui::ColorConvertFloat4ToU32(col4), type_label);
            ImGui::Dummy(ImVec2(cell_size.x, icon_size));
        }

        // Filename label — truncate if too long
        KStr filename = e.path.filename();
        filename = filename.split_at(10).first;

        // Inline rename
        if (renaming_idx == i) {
            ImGui::SetNextItemWidth(cell_size.x - 4);
            if (ImGui::InputText("##rename", rename_buf, sizeof(rename_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue |
                                     ImGuiInputTextFlags_AutoSelectAll)) {
                commit_rename();
            }
            if (!ImGui::IsItemActive() && !ImGui::IsItemActivated())
                renaming_idx = -1;  // clicked away
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text,
                                  ImVec4(0.85f, 0.85f, 0.85f, 1.f));
            // Centre the text
            float text_w =
                std::min(ImGui::CalcTextSize(filename.data(), filename.end()).x,
                         cell_size.x);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
                                 (cell_size.x - text_w) * 0.5f);
            ImGui::TextUnformatted(filename.data(), filename.end());
            ImGui::PopStyleColor();
        }

        ImGui::EndGroup();
        ImGui::PopID();

        col++;
    }

    ImGui::PopStyleVar();
}

// ── List view ────────────────────────────────────────────
void AssetBrowser::draw_list() {
    std::string filter = search_buf;

    for (int i = 0; i < (int)entries.size(); i++) {
        auto &e = entries[i];

        // Filter
        if (!filter.empty()) {
            KStr name = e.path.filename();
            if (name.find_first(filter) == -1) continue;
        }

        ImGui::PushID(i);
        bool is_selected = (i == selected_idx);

        // Icon
        ImVec4 col4 = asset_type_color(e.type);
        ImGui::PushStyleColor(ImGuiCol_Text, col4);
        ImGui::TextUnformatted(asset_type_icon(e.type));
        ImGui::PopStyleColor();
        ImGui::SameLine(0, 6);

        // Inline rename
        if (renaming_idx == i) {
            ImGui::SetNextItemWidth(300);
            if (ImGui::InputText("##rename", rename_buf, sizeof(rename_buf),
                                 ImGuiInputTextFlags_EnterReturnsTrue |
                                     ImGuiInputTextFlags_AutoSelectAll)) {
                commit_rename();
            }
        } else {
            KStr filename = e.path.filename();
            if (ImGui::Selectable(filename.data(), is_selected,
                                  ImGuiSelectableFlags_AllowDoubleClick)) {
                if (e.is_dir && ImGui::IsMouseDoubleClicked(0))
                    navigate_to(e.path.to_str());
                else
                    selected_idx = i;
            }

            // Right-click
            if (ImGui::BeginPopupContextItem("##ctx")) {
                selected_idx = i;
                draw_entry_context_menu(i);
                ImGui::EndPopup();
            }

            // Drag source
            if (!e.is_dir && ImGui::BeginDragDropSource()) {
                KStr path_str = e.path.to_str();
                ImGui::SetDragDropPayload("ASSET_PATH", path_str.data(),
                                          path_str.length() + 1);
                ImGui::TextUnformatted(filename.data());
                ImGui::EndDragDropSource();
            }
        }

        // Extension column (right aligned)
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.f));
        if (!e.is_dir) ImGui::TextUnformatted(e.path.extension().data());
        ImGui::PopStyleColor();

        ImGui::PopID();
    }
}

// ── Context menu ─────────────────────────────────────────
void AssetBrowser::draw_entry_context_menu(int idx) {
    auto &e = entries[idx];
    KStr filename = e.path.filename();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.8f, 1.f, 1.f));
    ImGui::TextUnformatted(filename.data());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (e.is_dir) {
        if (ImGui::MenuItem("Open")) navigate_to(e.path.to_str());
        ImGui::Separator();
    } else {
        if (ImGui::MenuItem("Open")) {
        }  // emit open event
        if (ImGui::MenuItem("Show in Explorer")) {
        }  // platform open
        ImGui::Separator();
    }

    if (ImGui::MenuItem("Rename")) {
        begin_rename(idx);
        ImGui::CloseCurrentPopup();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.4f, 0.4f, 1.f));
    if (ImGui::MenuItem("Delete")) {
        // fs::remove_all(e.path);
        needs_refresh = true;
        if (selected_idx == idx) selected_idx = -1;
    }
    ImGui::PopStyleColor();
}

// ── Rename ───────────────────────────────────────────────
void AssetBrowser::begin_rename(int idx) {
    renaming_idx = idx;
    KStr name = entries[idx].path.filename();
    strncpy(rename_buf, name.data(), sizeof(rename_buf) - 1);
    rename_buf[sizeof(rename_buf) - 1] = '\0';
    ImGui::SetKeyboardFocusHere();
}

void AssetBrowser::commit_rename() {
    if (renaming_idx < 0 || renaming_idx >= (int)entries.size()) return;
    auto &e = entries[renaming_idx];
    Path new_path = e.path.parent();
    new_path.push(rename_buf);
    // if (!fs::exists(new_path) && rename_buf[0] != '\0') {
    //     fs::rename(e.path, new_path);
    //     needs_refresh = true;
    // }
    renaming_idx = -1;
}

}  // namespace Seed