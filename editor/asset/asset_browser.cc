#include "asset_browser.h"
#include <fmt/base.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>
#include "asset.h"
#include "core/concurrency/thread_pool.h"
#include "core/container/kstring.h"
#include "core/engine.h"
#include "core/input.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/math/vec2.h"
#include "core/misc/uuid.h"
#include "core/project.h"
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
    if (ext == ".world")  // adjust to your format
        return AssetType::World;
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
        case AssetType::World:
            return "[W]";
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
        case AssetType::World:
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
    while (true) {
        breadcrumbs.insert(breadcrumbs.begin(), p);
        if (p == root_dir->get_path()) break;
        Path parent = p.parent();
        if (parent == p) break;
        p = p.parent();
    }
}

void AssetBrowser::invalidate_current_folder_cache() {
    if (!current_dir.is_valid()) return;
    folder_entry_cache.erase(current_dir->get_path());
}

void AssetBrowser::refresh() {
    entries.clear();
    if (!current_dir.is_valid()) return;

    Path folder_path = current_dir->get_path();
    auto cached = folder_entry_cache.find(folder_path);
    if (cached != folder_entry_cache.end()) {
        entries = cached->second;
        if (selected_idx >= (i32)entries.size()) selected_idx = -1;
        needs_refresh = false;
        return;
    }

    // Dirs first, then files, both sorted alphabetically
    std::vector<Path> dirs, files;
    std::vector<Path> childrens = current_dir->list();
    for (auto &path : childrens) {
        path = current_dir->concat(path);
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
        });
    for (auto &f : files) {
        AssetType type = classify(f);
        AssetEntry entry{
            .path = std::move(f),
            .type = type,
        };
        if (entry.type == AssetType::Texture) request_texture_thumbnail(entry);
        entries.push_back(std::move(entry));
    }

    folder_entry_cache[folder_path] = entries;
    if (selected_idx >= (i32)entries.size()) selected_idx = -1;
    needs_refresh = false;
}

void AssetBrowser::request_texture_thumbnail(AssetEntry &entry) {
    if (entry.thumbnail_requested || entry.thumbnail_failed ||
        !entry.thumbnail_texture.is_null())
        return;

    constexpr u32 max_preview_edge = 96;
    entry.thumbnail_requested = true;

    struct ThumbnailRequest {
            AssetBrowser *browser;
            Path path;
    };

    ThreadPool *pool = ThreadPool::get_instance();
    if (pool == nullptr) {
        Ref<Image> original = Image::load_from_file(entry.path, true);
        if (original.is_null()) {
            entry.thumbnail_failed = true;
            entry.thumbnail_requested = false;
            return;
        }

        Ref<Image> preview =
            original->downscale(max_preview_edge, max_preview_edge);
        entry.texture_width = original->get_width();
        entry.texture_height = original->get_height();
        entry.thumbnail_texture = preview->create_texture(
            SamplerProperty{.min_filter = SamplerFilter::LINEAR,
                            .mag_filter = SamplerFilter::LINEAR,
                            .wrap_u = SamplerWrap::CLAMP_TO_EDGE,
                            .wrap_v = SamplerWrap::CLAMP_TO_EDGE});
        return;
    }

    ThumbnailRequest *request = new ThumbnailRequest{this, entry.path};
    pool->add_work(
        [](void *data) {
            ThumbnailRequest *request = (ThumbnailRequest *)data;
            ThumbnailResult result;
            result.path = request->path;

            Ref<Image> original = Image::load_from_file(request->path, true);
            if (original.is_null()) {
                result.failed = true;
            } else {
                result.source_width = original->get_width();
                result.source_height = original->get_height();
                result.image =
                    original->downscale(max_preview_edge, max_preview_edge);
                result.failed = result.image.is_null();
            }

            request->browser->queue_thumbnail_result(std::move(result));
            delete request;
        },
        request);
}

void AssetBrowser::queue_thumbnail_result(ThumbnailResult result) {
    std::lock_guard lock(thumbnail_results_mutex);
    thumbnail_results.push_back(std::move(result));
}

bool AssetBrowser::apply_thumbnail_result(
    std::vector<AssetEntry> &target_entries, const ThumbnailResult &result,
    Ref<Texture> texture) {
    for (AssetEntry &entry : target_entries) {
        if (entry.path != result.path) continue;

        entry.thumbnail_requested = false;
        entry.thumbnail_failed = result.failed || texture.is_null();
        entry.texture_width = result.source_width;
        entry.texture_height = result.source_height;
        if (!texture.is_null()) entry.thumbnail_texture = texture;
        return true;
    }
    return false;
}

void AssetBrowser::process_thumbnail_results() {
    std::vector<ThumbnailResult> results;
    {
        std::lock_guard lock(thumbnail_results_mutex);
        results.swap(thumbnail_results);
    }

    SamplerProperty thumbnail_sampler{
        .min_filter = SamplerFilter::LINEAR,
        .mag_filter = SamplerFilter::LINEAR,
        .wrap_u = SamplerWrap::CLAMP_TO_EDGE,
        .wrap_v = SamplerWrap::CLAMP_TO_EDGE};

    for (const ThumbnailResult &result : results) {
        Ref<Texture> texture;
        if (!result.failed && !result.image.is_null()) {
            texture = result.image->create_texture(thumbnail_sampler);
        }

        apply_thumbnail_result(entries, result, texture);

        for (auto &[_, cached_entries] : folder_entry_cache) {
            apply_thumbnail_result(cached_entries, result, texture);
        }
    }
}

void AssetBrowser::draw_asset_option_menu() {
    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("World")) {
            gEditor->set_current_popup(new WorldCreatePopup);
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
}

// ── Main update ──────────────────────────────────────────
void AssetBrowser::update() {
    process_thumbnail_results();
    if (needs_refresh) refresh();

    draw_toolbar();
    ImGui::Separator();
    draw_breadcrumb();
    ImGui::Separator();

    // Main content area
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const float splitter_w = 4.f;
    const float min_folder_w = 150.f;
    const float min_asset_w = 180.f;
    float max_folder_w =
        std::max(min_folder_w, avail.x - splitter_w - min_asset_w);
    folder_panel_width =
        std::clamp(folder_panel_width, min_folder_w, max_folder_w);

    ImGui::BeginChild("##ab_folders", ImVec2(folder_panel_width, avail.y),
                      true);
    draw_folder_panel();
    ImGui::EndChild();

    ImGui::SameLine(0, 0);
    ImVec2 splitter_pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##ab_splitter", ImVec2(splitter_w, avail.y));
    if (ImGui::IsItemActive()) {
        folder_panel_width =
            std::clamp(folder_panel_width + ImGui::GetIO().MouseDelta.x,
                       min_folder_w, max_folder_w);
    }
    if (ImGui::IsItemHovered() || ImGui::IsItemActive())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    ImGui::GetWindowDrawList()->AddRectFilled(
        splitter_pos,
        ImVec2(splitter_pos.x + splitter_w, splitter_pos.y + avail.y),
        IM_COL32(55, 55, 55, 255));

    ImGui::SameLine(0, 0);
    ImGui::BeginChild("##ab_assets", ImVec2(0, avail.y), false);
    draw_grid();
    handle_external_drop_target();
    draw_empty_space_context_menu();
    ImGui::EndChild();
}

// ── Toolbar ──────────────────────────────────────────────
void AssetBrowser::draw_toolbar() {
    // Search box
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##ab_search", "Search...", search_buf,
                             sizeof(search_buf));

    ImGui::SameLine(0, 12);

    // Refresh button
    if (ImGui::SmallButton("Refresh")) {
        invalidate_current_folder_cache();
        needs_refresh = true;
    }

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
        invalidate_current_folder_cache();
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

void AssetBrowser::draw_folder_panel() {
    ImGui::TextUnformatted("Folders");
    ImGui::Separator();
    if (!root_dir.is_valid()) return;
    draw_folder_tree(root_dir->get_path());
}

void AssetBrowser::draw_folder_tree(const Path &dir) {
    std::vector<Path> child_dirs = list_child_directories(dir);
    bool selected = current_dir.is_valid() && dir == current_dir->get_path();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (selected) flags |= ImGuiTreeNodeFlags_Selected;
    if (dir == root_dir->get_path()) flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (child_dirs.empty())
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    KStr label = dir.filename();
    if (label.is_empty()) label = "Assets";

    ImGui::PushID(dir.data());
    bool open = ImGui::TreeNodeEx("##folder", flags, "%.*s",
                                  (int)label.length(), label.data());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) navigate_to(dir.to_str());

    if (open && !child_dirs.empty()) {
        for (const Path &child : child_dirs) draw_folder_tree(child);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

std::vector<Path> AssetBrowser::list_child_directories(const Path &dir) {
    std::vector<Path> dirs;
    Ref<Dir> d = Dir::open(dir.to_str());
    if (!d.is_valid()) return dirs;

    for (Path &child : d->list()) {
        child = d->concat(child);
        if (child.is_directory()) dirs.push_back(std::move(child));
    }
    std::sort(dirs.begin(), dirs.end());
    return dirs;
}

bool AssetBrowser::matches_search(const Path &path, KStr filter) {
    if (filter.is_empty()) return true;

    KString name = path.filename().string();
    name.to_lower();
    KString needle = filter.string();
    needle.to_lower();
    return KStr(name).find_first(KStr(needle)) != -1;
}

Path AssetBrowser::get_project_asset_path(const AssetEntry &entry) const {
    Project *project = SeedEngine::get_instance()->get_project();
    if (project == nullptr) return entry.path;
    if (entry.path.is_absolute())
        return entry.path.relative(project->get_path());
    return entry.path;
}

UUID AssetBrowser::get_asset_uuid(AssetEntry &entry) {
    Path _p = get_project_asset_path(entry);

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
    ResourceEntry *rentry =
        ResourceLoader::get_instance()->get_entries().get_entry(
            get_asset_uuid(entry));
    if (rentry == nullptr) return nullptr;
    switch (entry.type) {
        case AssetType::Mesh:
            return new ModelInspector(rentry->config);
        default:
            return nullptr;
    }
    return nullptr;
}

void AssetBrowser::open_asset(AssetEntry &entry) {
    if (entry.type == AssetType::Directory) {
        navigate_to(entry.path.to_str());
        return;
    }

    if (entry.type == AssetType::World) {
        gEditor->world_editor.load_world(get_project_asset_path(entry));
        return;
    }

    gEditor->set_current_inspect(create_inspectable(entry));
}

static void draw_centered_wrapped_text(KStr text, ImVec2 min, ImVec2 max,
                                       int max_lines) {
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    const char *text_begin = text.data();
    const char *text_end = text.end();
    const float line_height = ImGui::GetTextLineHeight();
    const float wrap_width = max.x - min.x;
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);

    const char *line_begin = text_begin;
    for (int line = 0; line < max_lines && line_begin < text_end; line++) {
        while (line_begin < text_end &&
               (*line_begin == ' ' || *line_begin == '\n' ||
                *line_begin == '\r')) {
            line_begin++;
        }
        if (line_begin >= text_end) break;

        const char *line_end = line_begin;
        const char *best_fit = line_begin;
        const char *best_separator_break = nullptr;
        for (const char *cursor = line_begin; cursor < text_end;) {
            const char *next = cursor + 1;
            while (next < text_end && ((*next & 0xC0) == 0x80)) next++;

            ImVec2 text_size = ImGui::CalcTextSize(line_begin, next);
            if (text_size.x > wrap_width && best_fit > line_begin) break;

            best_fit = next;
            bool is_separator = *cursor == '_' || *cursor == '-' ||
                                *cursor == ' ' || *cursor == '.';
            bool is_leading_dot = cursor == text_begin && *cursor == '.';
            if (is_separator && !is_leading_dot) {
                const char *candidate = *cursor == '.' ? cursor : next;
                if (candidate > line_begin) best_separator_break = candidate;
            }

            cursor = next;
        }

        line_end = best_fit;
        if (best_separator_break != nullptr &&
            best_separator_break > line_begin)
            line_end = best_separator_break;
        if (line_end <= line_begin) {
            line_end = line_begin + 1;
            while (line_end < text_end && ((*line_end & 0xC0) == 0x80))
                line_end++;
        }

        const char *draw_end = line_end;
        while (draw_end > line_begin &&
               (draw_end[-1] == ' ' || draw_end[-1] == '\n' ||
                draw_end[-1] == '\r')) {
            draw_end--;
        }

        ImVec2 text_size = ImGui::CalcTextSize(line_begin, draw_end);
        float x = min.x + std::max(0.f, (wrap_width - text_size.x) * 0.5f);
        float y = min.y + line * line_height;
        if (y + line_height > max.y) break;

        draw_list->AddText(ImVec2(x, y), color, line_begin, draw_end);
        line_begin = line_end;
    }
}

static void draw_checkerboard(ImDrawList *draw_list, ImVec2 min, ImVec2 max,
                              float tile_size) {
    const ImU32 col_a = IM_COL32(58, 58, 58, 255);
    const ImU32 col_b = IM_COL32(82, 82, 82, 255);
    int row = 0;
    for (float y = min.y; y < max.y; y += tile_size, row++) {
        int col = row & 1;
        for (float x = min.x; x < max.x; x += tile_size, col++) {
            ImVec2 tile_min(x, y);
            ImVec2 tile_max(std::min(x + tile_size, max.x),
                            std::min(y + tile_size, max.y));
            draw_list->AddRectFilled(tile_min, tile_max,
                                     (col & 1) ? col_a : col_b);
        }
    }
}

void AssetBrowser::draw_icon(AssetEntry &e, Vec2 cell_size, u32 rename_idx) {
    ImGui::BeginGroup();

    // Icon area
    ImVec4 col4 = asset_type_color(e.type);
    ImVec2 cell_pos = ImGui::GetCursorScreenPos();
    ImVec2 icon_pos(cell_pos.x + (cell_size.x - icon_size) * 0.5f, cell_pos.y);
    float icon_pad = 8.f;
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(icon_pos.x + icon_pad, icon_pos.y),
        ImVec2(icon_pos.x + icon_size - icon_pad, icon_pos.y + icon_size),
        ImGui::ColorConvertFloat4ToU32(
            ImVec4(col4.x * 0.3f, col4.y * 0.3f, col4.z * 0.3f, 1.f)),
        4.f);

    ImVec2 icon_min(icon_pos.x + icon_pad, icon_pos.y);
    ImVec2 icon_max(icon_pos.x + icon_size - icon_pad, icon_pos.y + icon_size);

    // If thumbnail is available draw it, otherwise draw type label
    if (!e.thumbnail_texture.is_null()) {
        draw_checkerboard(ImGui::GetWindowDrawList(), icon_min, icon_max, 6.f);
        u32 preview_w = e.thumbnail_texture->get_width();
        u32 preview_h = e.thumbnail_texture->get_height();
        float fit_scale =
            std::min((icon_max.x - icon_min.x) / (float)preview_w,
                     (icon_max.y - icon_min.y) / (float)preview_h);
        ImVec2 image_size(preview_w * fit_scale, preview_h * fit_scale);
        ImVec2 image_pos(
            icon_min.x + (icon_max.x - icon_min.x - image_size.x) * 0.5f,
            icon_min.y + (icon_max.y - icon_min.y - image_size.y) * 0.5f);
        ImGui::SetCursorScreenPos(image_pos);
        ImGui::Image((ImTextureID)(u64)e.thumbnail_texture->get_handle(),
                     image_size);
        ImGui::GetWindowDrawList()->AddRect(icon_min, icon_max,
                                            IM_COL32(25, 25, 25, 180), 3.f);
    } else {
        const char *type_label = asset_type_icon(e.type);
        ImVec2 tl_size = ImGui::CalcTextSize(type_label);
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(icon_pos.x + (icon_size - tl_size.x) * 0.5f,
                   icon_pos.y + (icon_size - tl_size.y) * 0.5f),
            ImGui::ColorConvertFloat4ToU32(col4), type_label);
    }
    ImGui::SetCursorScreenPos(cell_pos);
    ImGui::Dummy(ImVec2(cell_size.x, icon_size));

    KStr filename = e.path.filename();
    float label_gap = 6.f;
    float label_pad = 4.f;
    float label_h = cell_size.y - icon_size - label_gap;
    ImVec2 label_min(cell_pos.x + label_pad,
                     cell_pos.y + icon_size + label_gap);
    ImVec2 label_max(cell_pos.x + cell_size.x - label_pad,
                     label_min.y + label_h);

    // Inline rename
    if (renaming_idx == rename_idx) {
        ImGui::SetCursorScreenPos(label_min);
        ImGui::SetNextItemWidth(label_max.x - label_min.x);
        if (ImGui::InputText("##rename", rename_buf, sizeof(rename_buf),
                             ImGuiInputTextFlags_EnterReturnsTrue |
                                 ImGuiInputTextFlags_AutoSelectAll)) {
            commit_rename();
        }
        if (!ImGui::IsItemActive() && !ImGui::IsItemActivated())
            renaming_idx = -1;  // clicked away
    } else {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.f));
        ImGui::PushClipRect(label_min, label_max, true);
        draw_centered_wrapped_text(filename, label_min, label_max, 3);
        ImGui::PopClipRect();
        ImGui::PopStyleColor();
    }

    ImGui::EndGroup();
}

// ── Grid view ────────────────────────────────────────────
void AssetBrowser::draw_grid() {
    KStr filter = search_buf;

    float cell_w = 156.f;
    float label_h = ImGui::GetTextLineHeight() * 3.f + 8.f;
    float cell_h = icon_size + label_h + 6.f;
    float padding = 8.f;
    float avail_w = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, (int)((avail_w + padding) / (cell_w + padding)));

    ImVec2 start = ImGui::GetCursorScreenPos();
    int visible_count = 0;

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    for (int i = 0; i < (int)entries.size(); i++) {
        auto &e = entries[i];

        if (!matches_search(e.path, filter)) continue;

        int row = visible_count / cols;
        int col = visible_count % cols;

        ImVec2 pos = ImVec2(start.x + col * (cell_w + padding),
                            start.y + row * (cell_h + padding));

        ImGui::PushID(i);

        bool is_selected = (i == selected_idx);
        ImVec2 cell_size = ImVec2(cell_w, cell_h);
        ImGui::SetCursorScreenPos(pos);
        ImVec2 cursor = ImGui::GetCursorScreenPos();

        // Selection highlight
        if (is_selected) {
            ImGui::GetWindowDrawList()->AddRectFilled(
                cursor, ImVec2(cursor.x + cell_size.x, cursor.y + cell_size.y),
                IM_COL32(60, 100, 160, 180), 4.f);
        }

        // Invisible selectable covering the whole cell
        if (ImGui::InvisibleButton("##cell", cell_size)) {
            selected_idx = i;
            if (e.type != AssetType::Directory) open_asset(e);
        }
        bool cell_hovered = ImGui::IsItemHovered();
        bool tooltip_hovered =
            ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);

        // Double-click to open
        if (cell_hovered &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            selected_idx = i;
            open_asset(e);
        }

        // Hover highlight
        if (cell_hovered && !is_selected) {
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
        if (e.type != AssetType::Directory &&
            ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            UUID uuid = get_asset_uuid(e);
            if (!uuid.is_null()) {
                ImGui::SetDragDropPayload("UUID", &uuid, sizeof(uuid));
                ImGui::TextUnformatted(e.path.filename().data());
            }

            ImGui::EndDragDropSource();
        }

        if (tooltip_hovered) {
            Path asset_path = get_project_asset_path(e);
            KStr filename = e.path.filename();
            KStr asset_path_str = asset_path.to_str();
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(filename.data(), filename.end());
            if (e.type == AssetType::Texture && e.texture_width != 0 &&
                e.texture_height != 0) {
                ImGui::TextDisabled("%u x %u", e.texture_width,
                                    e.texture_height);
            }
            ImGui::TextDisabled("%.*s", (int)asset_path_str.length(),
                                asset_path_str.data());
            ImGui::EndTooltip();
        }

        // Draw icon and label on top of the selectable
        ImGui::SetCursorScreenPos(cursor);
        draw_icon(e, Vec2{cell_size.x, cell_size.y}, i);
        ImGui::PopID();

        visible_count++;
    }

    ImGui::PopStyleVar();

    if (visible_count == 0) {
        ImGui::SetCursorScreenPos(start);
        ImGui::TextDisabled("No assets");
        return;
    }

    int rows = (visible_count + cols - 1) / cols;
    float total_h = rows * cell_h + std::max(0, rows - 1) * padding;
    ImGui::SetCursorScreenPos(ImVec2(start.x, start.y + total_h));
    ImGui::Dummy(ImVec2(1.f, 1.f));
}

// ── Context menu ─────────────────────────────────────────
void AssetBrowser::draw_entry_context_menu(int idx) {
    auto &e = entries[idx];
    KStr filename = e.path.filename();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.8f, 1.f, 1.f));
    ImGui::TextUnformatted(filename.data());
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (e.type == AssetType::Directory) {
        if (ImGui::MenuItem("Open")) navigate_to(e.path.to_str());
        ImGui::Separator();
    } else {
        if (ImGui::MenuItem("Open")) {
            open_asset(e);
        }
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
        invalidate_current_folder_cache();
        needs_refresh = true;
        if (selected_idx == idx) selected_idx = -1;
    }
    ImGui::PopStyleColor();
}

void AssetBrowser::draw_empty_space_context_menu() {
    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selected_idx = -1;
    }

    if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
        ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        selected_idx = -1;
        ImGui::OpenPopup("##ab_empty_context");
    }

    if (ImGui::BeginPopup("##ab_empty_context")) {
        draw_asset_option_menu();
        ImGui::EndPopup();
    }
}

void AssetBrowser::handle_external_drop_target() {
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window == nullptr || window->SkipItems) return;

    ImRect drop_rect(window->Pos, ImVec2(window->Pos.x + window->Size.x,
                                         window->Pos.y + window->Size.y));
    if (!ImGui::BeginDragDropTargetCustom(drop_rect, window->ID)) return;

    if (auto p = ImGui::AcceptDragDropPayload("EXTERNAL")) {
        KStr data((char *)p->Data, (u32)p->DataSize - 1);
        std::vector<KStr> files = data.split("\n");
        Project *project = SeedEngine::get_instance()->get_project();
        Path target_dir =
            project != nullptr
                ? current_dir->get_path().relative(project->get_path())
                : current_dir->get_path();

        for (auto file : files) {
            if (file.is_empty()) continue;
            gEditor->import_asset(file, target_dir);
        }
        invalidate_current_folder_cache();
        needs_refresh = true;
    }
    ImGui::EndDragDropTarget();
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
