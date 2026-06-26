#include "asset_browser.h"
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
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/project.h"
#include "core/resource/resource.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "editor/editor.h"
#include "editor/gui/editor_ui.h"
#include "editor/project/preprocessor.h"

namespace Seed {
AssetType AssetBrowser::classify(const Path &p) {
    if (p.is_directory()) return AssetType::Directory;
    KString extension = p.extension().string().to_lower();
    KStr ext = extension;

    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
        ext == ".exr" || ext == ".hdr")
        return AssetType::Texture;
    if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".glb")
        return AssetType::Mesh;
    if (ext == ".world") return AssetType::World;
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

    SamplerProperty thumbnail_sampler{.min_filter = SamplerFilter::LINEAR,
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

void AssetBrowser::update() {
    process_thumbnail_results();
    if (needs_refresh) refresh();

    draw_toolbar();
    ImGui::Separator();
    draw_breadcrumb();
    ImGui::Separator();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    const f32 splitter_w = 4.f;
    const f32 min_folder_w = 150.f;
    const f32 min_asset_w = 180.f;
    f32 max_folder_w =
        std::max(min_folder_w, avail.x - splitter_w - min_asset_w);
    folder_panel_width =
        std::clamp(folder_panel_width, min_folder_w, max_folder_w);

    ImGui::BeginChild("##ab_folders", ImVec2(folder_panel_width, avail.y),
                      true);
    draw_folder_panel();
    ImGui::EndChild();

    ImGui::SameLine(0, 0);
    EditorUI::vertical_splitter("##ab_splitter", avail.y, splitter_w,
                                folder_panel_width, min_folder_w, max_folder_w);

    ImGui::SameLine(0, 0);
    ImGui::BeginChild("##ab_assets", ImVec2(0, avail.y), false);
    draw_grid();
    handle_external_drop_target();
    draw_empty_space_context_menu();
    ImGui::EndChild();
}

void AssetBrowser::draw_toolbar() {
    ImGui::SetNextItemWidth(200);
    ImGui::InputTextWithHint("##ab_search", "Search...", search_buf,
                             sizeof(search_buf));

    ImGui::SameLine(0, 12);
    if (ImGui::SmallButton("Refresh")) {
        invalidate_current_folder_cache();
        needs_refresh = true;
    }

    ImGui::SameLine(0, 12);
    if (ImGui::SmallButton("+ Folder")) {
        Path new_dir = current_dir->get_path();
        new_dir.push("New Folder");
        i32 suffix = 0;
        while (Dir::exists(new_dir)) {
            new_dir.pop();
            new_dir.push("New Folder " + std::to_string(++suffix));
        }
        Dir::create_if_not_exists(new_dir);
        invalidate_current_folder_cache();
        needs_refresh = true;
    }
}

void AssetBrowser::draw_breadcrumb() {
    EditorUI::ScopedStyleVar spacing(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
    for (i32 i = 0; i < (i32)breadcrumbs.size(); i++) {
        if (i > 0) {
            ImGui::SameLine();
            ImGui::TextDisabled(">");
            ImGui::SameLine();
        }
        KStr label = i == 0 ? root_dir->get_path().filename()
                            : breadcrumbs[i].filename();

        bool is_current = (breadcrumbs[i] == current_dir->get_path());
        if (is_current) {
            EditorUI::ScopedStyleColor text_color(
                ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.f));
            ImGui::TextUnformatted(label.data(), label.end());
        } else {
            EditorUI::ScopedStyleColor text_color(
                ImGuiCol_Text, ImVec4(0.5f, 0.7f, 0.9f, 1.f));
            if (ImGui::SmallButton(label.data()))
                navigate_to(breadcrumbs[i].to_str());
        }
    }
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

    EditorUI::ScopedID folder_id(dir.data());
    bool open = ImGui::TreeNodeEx("##folder", flags, "%.*s",
                                  (i32)label.length(), label.data());
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) navigate_to(dir.to_str());

    if (open && !child_dirs.empty()) {
        for (const Path &child : child_dirs) draw_folder_tree(child);
        ImGui::TreePop();
    }
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

AssetBrowser::GridLayout AssetBrowser::make_grid_layout() const {
    GridLayout layout;
    layout.cell_width = 156.f;
    layout.padding = 8.f;
    f32 label_h = ImGui::GetTextLineHeight() * 3.f + 8.f;
    layout.cell_height = icon_size + label_h + 6.f;
    f32 avail_w = ImGui::GetContentRegionAvail().x;
    layout.columns = std::max(1, (i32)((avail_w + layout.padding) /
                                       (layout.cell_width + layout.padding)));
    layout.start = ImGui::GetCursorScreenPos();
    return layout;
}

AssetBrowser::AssetCellRects AssetBrowser::make_asset_cell_rects(
    ImVec2 cell_pos, ImVec2 cell_size) const {
    static constexpr f32 ICON_PAD = 8.f;
    static constexpr f32 LABEL_GAP = 6.f;
    static constexpr f32 LABEL_PAD = 4.f;

    AssetCellRects rects;
    rects.min = cell_pos;
    rects.max = ImVec2(cell_pos.x + cell_size.x, cell_pos.y + cell_size.y);

    ImVec2 icon_pos(cell_pos.x + (cell_size.x - icon_size) * 0.5f, cell_pos.y);
    rects.icon_min = ImVec2(icon_pos.x + ICON_PAD, icon_pos.y);
    rects.icon_max =
        ImVec2(icon_pos.x + icon_size - ICON_PAD, icon_pos.y + icon_size);

    f32 label_h = cell_size.y - icon_size - LABEL_GAP;
    rects.label_min =
        ImVec2(cell_pos.x + LABEL_PAD, cell_pos.y + icon_size + LABEL_GAP);
    rects.label_max = ImVec2(cell_pos.x + cell_size.x - LABEL_PAD,
                             rects.label_min.y + label_h);
    return rects;
}

void AssetBrowser::draw_asset_label(AssetEntry &entry,
                                    const AssetCellRects &rects, i32 idx) {
    if (renaming_idx == idx) {
        ImGui::SetCursorScreenPos(rects.label_min);
        ImGui::SetNextItemWidth(rects.label_max.x - rects.label_min.x);
        if (ImGui::InputText("##rename", rename_buf, sizeof(rename_buf),
                             ImGuiInputTextFlags_EnterReturnsTrue |
                                 ImGuiInputTextFlags_AutoSelectAll)) {
            commit_rename();
        }
        if (!ImGui::IsItemActive() && !ImGui::IsItemActivated())
            renaming_idx = -1;
        return;
    }

    EditorUI::ScopedStyleColor text_color(ImGuiCol_Text,
                                          ImVec4(0.85f, 0.85f, 0.85f, 1.f));
    EditorUI::ScopedClipRect clip(rects.label_min, rects.label_max, true);
    EditorUI::draw_centered_wrapped_text(entry.path.filename(), rects.label_min,
                                         rects.label_max, 3);
}

void AssetBrowser::draw_asset_visual(AssetEntry &entry,
                                     const AssetCellRects &rects, i32 idx) {
    EditorUI::ScopedGroup group;

    ImVec4 color = asset_type_color(entry.type);
    ImGui::GetWindowDrawList()->AddRectFilled(
        rects.icon_min, rects.icon_max,
        ImGui::ColorConvertFloat4ToU32(
            ImVec4(color.x * 0.3f, color.y * 0.3f, color.z * 0.3f, 1.f)),
        4.f);

    if (!entry.thumbnail_texture.is_null()) {
        EditorUI::TexturePreview preview;
        preview.texture = entry.thumbnail_texture;
        preview.width = entry.texture_width != 0
                            ? entry.texture_width
                            : entry.thumbnail_texture->get_width();
        preview.height = entry.texture_height != 0
                             ? entry.texture_height
                             : entry.thumbnail_texture->get_height();
        EditorUI::draw_texture_preview(&preview, rects.icon_min, rects.icon_max,
                                       false);
    } else {
        EditorUI::draw_centered_text(ImGui::GetWindowDrawList(),
                                     asset_type_icon(entry.type),
                                     rects.icon_min, rects.icon_max,
                                     ImGui::ColorConvertFloat4ToU32(color));
    }

    ImGui::SetCursorScreenPos(rects.min);
    ImGui::Dummy(ImVec2(rects.max.x - rects.min.x, icon_size));
    draw_asset_label(entry, rects, idx);
}

void AssetBrowser::draw_asset_tooltip(AssetEntry &entry) {
    Path asset_path = get_project_asset_path(entry);
    KStr filename = entry.path.filename();
    KStr asset_path_str = asset_path.to_str();
    ImGui::BeginTooltip();
    ImGui::TextUnformatted(filename.data(), filename.end());
    if (entry.type == AssetType::Texture && entry.texture_width != 0 &&
        entry.texture_height != 0) {
        ImGui::TextDisabled("%u x %u", entry.texture_width,
                            entry.texture_height);
    }
    ImGui::TextDisabled("%.*s", (i32)asset_path_str.length(),
                        asset_path_str.data());
    ImGui::EndTooltip();
}

void AssetBrowser::draw_asset_drag_source(AssetEntry &entry) {
    if (entry.type == AssetType::Directory) return;
    if (!ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        return;

    UUID uuid = get_asset_uuid(entry);
    if (!uuid.is_null()) {
        ImGui::SetDragDropPayload("UUID", &uuid, sizeof(uuid));
        ImGui::TextUnformatted(entry.path.filename().data());
    }

    ImGui::EndDragDropSource();
}

void AssetBrowser::draw_asset_cell(AssetEntry &entry, i32 idx,
                                   const AssetCellRects &rects) {
    bool is_selected = idx == selected_idx;
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 cell_size(rects.max.x - rects.min.x, rects.max.y - rects.min.y);

    if (is_selected) {
        draw_list->AddRectFilled(rects.min, rects.max,
                                 IM_COL32(60, 100, 160, 180), 4.f);
    }

    ImGui::SetCursorScreenPos(rects.min);
    if (ImGui::InvisibleButton("##cell", cell_size)) {
        selected_idx = idx;
        if (entry.type != AssetType::Directory) open_asset(entry);
    }

    bool cell_hovered = ImGui::IsItemHovered();
    bool tooltip_hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_ForTooltip);

    if (cell_hovered && !is_selected) {
        draw_list->AddRectFilled(rects.min, rects.max,
                                 IM_COL32(80, 80, 80, 120), 4.f);
    }

    if (cell_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        selected_idx = idx;
        open_asset(entry);
    }

    if (ImGui::BeginPopupContextItem("##ctx")) {
        selected_idx = idx;
        draw_entry_context_menu(idx);
        ImGui::EndPopup();
    }

    draw_asset_drag_source(entry);

    if (tooltip_hovered) {
        draw_asset_tooltip(entry);
    }

    ImGui::SetCursorScreenPos(rects.min);
    draw_asset_visual(entry, rects, idx);
}

void AssetBrowser::draw_grid() {
    KStr filter = search_buf;

    GridLayout layout = make_grid_layout();
    i32 visible_count = 0;

    EditorUI::ScopedStyleVar spacing(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    for (i32 i = 0; i < (i32)entries.size(); i++) {
        auto &e = entries[i];

        if (!matches_search(e.path, filter)) continue;

        i32 row = visible_count / layout.columns;
        i32 col = visible_count % layout.columns;

        ImVec2 pos = ImVec2(
            layout.start.x + col * (layout.cell_width + layout.padding),
            layout.start.y + row * (layout.cell_height + layout.padding));
        ImVec2 cell_size(layout.cell_width, layout.cell_height);

        EditorUI::ScopedID id(i);
        AssetCellRects rects = make_asset_cell_rects(pos, cell_size);
        draw_asset_cell(e, i, rects);

        visible_count++;
    }

    if (visible_count == 0) {
        ImGui::SetCursorScreenPos(layout.start);
        ImGui::TextDisabled("No assets");
        return;
    }

    i32 rows = (visible_count + layout.columns - 1) / layout.columns;
    f32 total_h =
        rows * layout.cell_height + std::max(0, rows - 1) * layout.padding;
    ImGui::SetCursorScreenPos(ImVec2(layout.start.x, layout.start.y + total_h));
    ImGui::Dummy(ImVec2(1.f, 1.f));
}

void AssetBrowser::draw_entry_context_menu(i32 idx) {
    auto &e = entries[idx];
    KStr filename = e.path.filename();

    {
        EditorUI::ScopedStyleColor text_color(ImGuiCol_Text,
                                              ImVec4(0.7f, 0.8f, 1.f, 1.f));
        ImGui::TextUnformatted(filename.data());
    }
    ImGui::Separator();

    if (e.type == AssetType::Directory) {
        if (ImGui::MenuItem("Open")) navigate_to(e.path.to_str());
        ImGui::Separator();
    } else {
        if (ImGui::MenuItem("Open")) {
            open_asset(e);
        }
        ImGui::Separator();
    }

    if (ImGui::MenuItem("Rename")) {
        begin_rename(idx);
        ImGui::CloseCurrentPopup();
    }
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

void AssetBrowser::begin_rename(i32 idx) {
    renaming_idx = idx;
    KStr name = entries[idx].path.filename();
    strncpy(rename_buf, name.data(), sizeof(rename_buf) - 1);
    rename_buf[sizeof(rename_buf) - 1] = '\0';
    ImGui::SetKeyboardFocusHere();
}

void AssetBrowser::commit_rename() {
    if (renaming_idx < 0 || renaming_idx >= (i32)entries.size()) return;
    renaming_idx = -1;
}

}  // namespace Seed
