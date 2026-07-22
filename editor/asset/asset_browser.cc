#include "asset_browser.h"
#include <algorithm>
#include <utility>
#include <vector>
#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Variant.h>
#include "asset.h"
#include "core/concurrency/thread_pool.h"
#include "core/container/kstring.h"
#include "core/engine.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/misc/uuid.h"
#include "core/project.h"
#include "core/resource/resource.h"
#include "core/resource/resource_entry.h"
#include "core/resource/resource_loader.h"
#include "core/resource/world_setting.h"
#include "core/serialize/json_impl.h"
#include "editor/editor.h"
#include "editor/project/preprocessor.h"
#include "core/gui/rml_widgets.h"
#include "core/input.h"
#include "core/gui/gui_engine.h"

namespace Seed {

namespace {

KString fit_asset_label(KStr name) {
    constexpr u32 max_label_length = 13;
    if (name.length() <= max_label_length) return name.string();

    i32 dot = name.find_last(".");
    bool has_extension = dot > 0 && dot < (i32)name.length() - 1 &&
                         name.length() - (u32)dot <= 5;
    if (!has_extension) {
        KString label;
        label.append(KStr(name.data(), max_label_length - 3));
        label.append("...");
        return label;
    }

    KStr extension(name.data() + dot, name.length() - (u32)dot);
    u32 prefix_length = max_label_length - 3 - extension.length();

    KString label;
    label.append(KStr(name.data(), prefix_length));
    label.append("...");
    label.append(extension);
    return label;
}

}  // namespace

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
            return "DIR";
        case AssetType::Texture:
            return "TEX";
        case AssetType::Mesh:
            return "MESH";
        case AssetType::World:
            return "WORLD";
        case AssetType::Material:
            return "MAT";
        case AssetType::Audio:
            return "AUD";
        case AssetType::Script:
            return "SRC";
        default:
            return "FILE";
    }
}

const char *AssetBrowser::asset_type_name(AssetType t) {
    switch (t) {
        case AssetType::Directory:
            return "directory";
        case AssetType::Texture:
            return "texture";
        case AssetType::Mesh:
            return "mesh";
        case AssetType::World:
            return "world";
        case AssetType::Material:
            return "material";
        case AssetType::Audio:
            return "audio";
        case AssetType::Script:
            return "script";
        default:
            return "file";
    }
}
AssetBrowser::AssetBrowser() {
    Ref<Image> default_preview;
    default_preview.create(PixelFormat::RGBA, 48, 48);
    default_preview->fill(Color{128, 128, 128, 255}, 48, 48);
    System::gGuiEngine->add_texture("preview-default",
                                    default_preview->create_texture());
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
            .uuid = UUID{},
            .path = std::move(d),
            .type = AssetType::Directory,
        });
    for (auto &f : files) {
        AssetType type = classify(f);
        AssetEntry entry{
            .path = std::move(f),
            .type = type,
        };
        entry.uuid = get_asset_uuid(entry);
        entries.push_back(std::move(entry));
    }

    folder_entry_cache[folder_path] = entries;
    if (selected_idx >= (i32)entries.size()) selected_idx = -1;
    needs_refresh = false;
}

void AssetBrowser::sync_view_model() {
    if (needs_refresh) refresh();

    asset_items.clear();
    folder_items.clear();
    folder_paths.clear();
    breadcrumb_text.clear();

    if (!root_dir.is_valid() || !current_dir.is_valid()) return;

    Path root_path = root_dir->get_path();
    Path current_path = current_dir->get_path();

    folder_paths.push_back(root_path);
    folder_items.push_back(AssetFolderView{
        .name = root_path.filename(),
        .active = current_path == root_path,
    });

    std::vector<Path> dirs = list_child_directories(root_path);
    for (Path &dir : dirs) {
        folder_paths.push_back(dir);
        folder_items.push_back(AssetFolderView{
            .name = dir.filename(),
            .active = current_path == dir,
        });
    }

    for (u32 i = 0; i < breadcrumbs.size(); i++) {
        if (i > 0) breadcrumb_text += " / ";
        breadcrumb_text += breadcrumbs[i].filename();
    }

    KStr filter(search_text);
    for (i32 i = 0; i < (i32)entries.size(); i++) {
        AssetEntry &entry = entries[i];
        if (!matches_search(entry.path, filter)) continue;

        asset_items.push_back(AssetItemView{
            .uuid = entry.uuid,
            .name = fit_asset_label(entry.path.filename()),
            .type = asset_type_name(entry.type),
            .icon = asset_type_icon(entry.type),
            .entry_index = i,
        });
    }
}

void AssetBrowser::dirty_view_model() {
    if (!asset_model) return;
    asset_model.DirtyVariable("assets");
    asset_model.DirtyVariable("folders");
    asset_model.DirtyVariable("breadcrumb");
    asset_model.DirtyVariable("search");
}

static void dirty_world_create_model(Rml::DataModelHandle model) {
    if (!model) return;
    model.DirtyVariable("show_create");
    model.DirtyVariable("name");
    model.DirtyVariable("path");
    model.DirtyVariable("error");
    model.DirtyVariable("has_error");
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
    Project *project = System::gEngine->get_project();
    if (project == nullptr) return entry.path;
    if (entry.path.is_absolute())
        return entry.path.relative(project->get_path());
    return entry.path;
}

Path AssetBrowser::current_asset_directory() const {
    Project *project = System::gEngine->get_project();
    if (project == nullptr || !current_dir.is_valid()) return Path("assets");

    Path dir = current_dir->get_path();
    if (dir.is_absolute()) return dir.relative(project->get_path());
    return dir;
}

bool AssetBrowser::create_world_asset() {
    if (new_world_name.empty()) {
        world_create_error = "World name is required.";
        has_world_create_error = true;
        return false;
    }

    Project *project = System::gEngine->get_project();
    if (project == nullptr) {
        world_create_error = "No project is loaded.";
        has_world_create_error = true;
        return false;
    }

    Path folder = new_world_path.empty() ? current_asset_directory()
                                         : Path(KStr(new_world_path));
    if (folder.is_absolute()) folder = folder.relative(project->get_path());

    KStr name(new_world_name);
    KString file_name = name.string();
    if (!name.end_with(".world")) file_name.append(".world");
    Path asset_path = folder.append(file_name);

    ResourceEntries *entries = System::gResourceEntries;
    if (!entries->get_uuid(asset_path).is_null()) {
        world_create_error = "World already exists.";
        has_world_create_error = true;
        return false;
    }

    UUID uuid = entries->insert_entry(asset_path, type_id<WorldSetting>());
    ResourceEntry *entry = entries->get_entry(uuid);
    if (entry == nullptr) {
        world_create_error = "Failed to create world.";
        has_world_create_error = true;
        return false;
    }

    nlohmann::ordered_json &j = entry->config.get_json();
    j["name"] = new_world_name;
    j["sky"] = {
        {"up", UUID{}},    {"down", UUID{}},  {"left", UUID{}},
        {"right", UUID{}}, {"front", UUID{}}, {"back", UUID{}},
    };
    j["directional_light"] = {
        {"enabled", true},
        {"direction", Vec3{-0.5f, -0.5f, 0.0f}},
        {"diffuse", Vec3{0.8f, 0.8f, 0.8f}},
        {"specular", Vec3{0.4f, 0.4f, 0.4f}},
    };
    j["chunks"] = nlohmann::ordered_json::array();

    entries->save(project->get_entry_path());
    System::gEditor->save_project();
    System::gEditor->world_editor.load_world(entry->uuid);

    invalidate_current_folder_cache();
    needs_refresh = true;
    sync_view_model();
    dirty_view_model();
    return true;
}

UUID AssetBrowser::get_asset_uuid(AssetEntry &entry) {
    Path _p = get_project_asset_path(entry);

    if (entry.type == AssetType::Mesh) {
        PreprocessEntry *pentry =
            System::gEditor->preprocessor.get_entry_from_path(_p);
        if (pentry == nullptr) return UUID();
        return pentry->target_uuid;
    } else {
        UUID uuid = System::gResourceEntries->get_uuid(_p);
        return uuid;
    }
}

// Inspectable *AssetBrowser::create_inspectable(AssetEntry &entry) {
//     ResourceEntry *rentry =
//         System::gResourceEntries->get_entry(
//             get_asset_uuid(entry));
//     if (rentry == nullptr) return nullptr;
//     switch (entry.type) {
//         case AssetType::Mesh:
//             return new ModelInspector(rentry->config);
//         default:
//             return nullptr;
//     }
//     return nullptr;
// }

void AssetBrowser::open_asset(AssetEntry &entry) {
    if (entry.type == AssetType::Directory) {
        navigate_to(entry.path.to_str());
        return;
    }

    if (entry.type == AssetType::World) {
        System::gEditor->world_editor.load_world(get_asset_uuid(entry));
        return;
    }
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

void AssetBrowser::rml_refresh(RML_EVENT_ARGS) {
    asset_model = model;

    if (e.GetType() == "change") {
        Rml::String value = e.GetParameter<Rml::String>("value", "");
        search_text = value.c_str();
        sync_view_model();
        dirty_view_model();
        return;
    }

    invalidate_current_folder_cache();
    needs_refresh = true;
    sync_view_model();
    dirty_view_model();
}

void AssetBrowser::rml_open_asset(RML_EVENT_ARGS) {
    i32 item_index = args.empty() ? -1 : args[0].Get<int>(-1);
    if (item_index < 0 || item_index >= (i32)asset_items.size()) return;

    i32 entry_index = asset_items[item_index].entry_index;
    if (entry_index < 0 || entry_index >= (i32)entries.size()) return;

    open_asset(entries[entry_index]);
    sync_view_model();
    asset_model = model;
    dirty_view_model();
}

void AssetBrowser::rml_open_folder(RML_EVENT_ARGS) {
    i32 folder_index = args.empty() ? -1 : args[0].Get<int>(-1);
    if (folder_index < 0 || folder_index >= (i32)folder_paths.size()) return;

    navigate_to(folder_paths[folder_index].to_str());
    sync_view_model();
    asset_model = model;
    dirty_view_model();
}

void AssetBrowser::rml_open_menu(RML_EVENT_ARGS) {
    if (!System::gInput->is_mouse_released(MouseEvent::RIGHT)) {
        return;
    }
    Rml::Element *_menu = document->GetElementById("asset-popup");

    auto *menu = dynamic_cast<RmlMenu *>(_menu);
    if (menu == nullptr) return;

    float x = e.GetParameter<float>("mouse_x", 0.0f);
    float y = e.GetParameter<float>("mouse_y", 0.0f);
    menu->open(x, y);
}

void AssetBrowser::rml_request_create_world(RML_EVENT_ARGS) {
    new_world_name.clear();
    Path asset_dir = current_asset_directory();
    new_world_path =
        std::string(asset_dir.to_str().data(), asset_dir.to_str().length());
    world_create_error.clear();
    has_world_create_error = false;
    show_world_create = true;
    dirty_world_create_model(world_create_model);
}

void AssetBrowser::rml_cancel_create_world(RML_EVENT_ARGS) {
    show_world_create = false;
    world_create_error.clear();
    has_world_create_error = false;
    dirty_world_create_model(world_create_model);
}

void AssetBrowser::rml_confirm_create_world(RML_EVENT_ARGS) {
    if (!create_world_asset()) {
        dirty_world_create_model(world_create_model);
        return;
    }

    show_world_create = false;
    new_world_name.clear();
    world_create_error.clear();
    has_world_create_error = false;
    dirty_world_create_model(world_create_model);
}

void AssetBrowser::bind_model(Rml::Context *context) {
    Rml::DataModelConstructor constructor =
        context->CreateDataModel("asset_browser");
    if (!constructor) return;

    if (auto item = constructor.RegisterStruct<AssetItemView>()) {
        item.RegisterMember("name", &AssetItemView::name);
        item.RegisterMember("type", &AssetItemView::type);
        item.RegisterMember("icon", &AssetItemView::icon);
        item.RegisterMember("uuid", &AssetItemView::uuid);
    }
    if (auto folder = constructor.RegisterStruct<AssetFolderView>()) {
        folder.RegisterMember("name", &AssetFolderView::name);
        folder.RegisterMember("active", &AssetFolderView::active);
    }

    constructor.RegisterArray<std::vector<AssetItemView>>();
    constructor.RegisterArray<std::vector<AssetFolderView>>();
    constructor.Bind("assets", &asset_items);
    constructor.Bind("folders", &folder_items);
    constructor.Bind("breadcrumb", &breadcrumb_text);
    constructor.Bind("search", &search_text);
    constructor.BindEventCallback("refresh_assets", &AssetBrowser::rml_refresh,
                                  this);
    constructor.BindEventCallback("open_asset", &AssetBrowser::rml_open_asset,
                                  this);
    constructor.BindEventCallback("open_folder", &AssetBrowser::rml_open_folder,
                                  this);
    constructor.BindEventCallback("open_menu", &AssetBrowser::rml_open_menu,
                                  this);
    constructor.BindEventCallback(
        "request_create_world", &AssetBrowser::rml_request_create_world, this);

    asset_model = constructor.GetModelHandle();
    sync_view_model();
    dirty_view_model();

    if (Rml::DataModelConstructor constructor =
            context->CreateDataModel("world_create")) {
        constructor.Bind("show_create", &show_world_create);
        constructor.Bind("name", &new_world_name);
        constructor.Bind("path", &new_world_path);
        constructor.Bind("error", &world_create_error);
        constructor.Bind("has_error", &has_world_create_error);
        constructor.BindEventCallback("cancel_create_world",
                                      &AssetBrowser::rml_cancel_create_world,
                                      this);
        constructor.BindEventCallback("confirm_create_world",
                                      &AssetBrowser::rml_confirm_create_world,
                                      this);

        world_create_model = constructor.GetModelHandle();
        dirty_world_create_model(world_create_model);
    }
}

}  // namespace Seed
