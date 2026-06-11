
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/project.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/model.h"
#include "core/resource/resource_entry.h"
#include "core/resource/world_setting.h"
#include <nfd.h>
#include <fmt/format.h>
#include "core/engine.h"
#include "core/gui/gui_engine.h"
#include "core/resource/resource_loader.h"
#include "camera_entity.h"
#include "editor.h"
#include "editor_storage.h"
#include "core/serialize/json_impl.h"
#include <queue>
#include "core/concurrency/thread_pool.h"

namespace Seed {
Editor *gEditor = nullptr;

Editor::Editor() {
    gEditor = this;
    Ref<File> cache = File::open(".seed_cache", "rb");
    SeedEngine *engine = SeedEngine::get_instance();
    if (cache.is_valid()) {
        project_cache = cache->read_json();
        if (project_cache.contains("last_open")) {
            engine->load_project(project_cache["last_open"]);
        }
    }
    new EditorStorage;
    GuiEngine::get_instance()->add_gui(&editor_gui);
    world_editor.init();
    asset_viewer.init();
    Project *project = engine->get_project();
    if (engine->get_project()) {
        preprocessor.init(project->get_asset_dir());
        if (project->get_preprocess_entry_path().is_file()) {
            preprocessor.get_entries().load(
                project->get_preprocess_entry_path());
        }
        asset_browser.init(project->get_asset_dir());
        if (project_cache.contains("last_open_world")) {
            world_editor.load_world(project_cache["last_open_world"]);
        }
    }
}

void Editor::set_last_open(const Path &path) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open"] = path;
    cache->write_str(project_cache.dump());
}

void Editor::set_last_open_world(const Path &path) {
    Ref<File> cache = File::open(".seed_cache", "wb");
    project_cache["last_open_world"] = path;
    cache->write_str(project_cache.dump());
}

void Editor::set_current_inspect(Inspectable *inspectable) {
    Project *project = SeedEngine::get_instance()->get_project();

    if (this->ctx.current_inspect != nullptr) {
        this->ctx.current_inspect->save();
        delete this->ctx.current_inspect;
        ResourceLoader::get_instance()->get_entries().save(
            project->get_entry_path());
    }
    this->ctx.current_inspect = inspectable;
}

void Editor::set_current_popup(Popup *popup) {
    if (this->ctx.current_popup != nullptr) {
        delete this->ctx.current_popup;
        // ResourceLoader::get_instance()->get_entries().save(
        //     current_project->get_entry_path());
    }
    this->ctx.current_popup = popup;
}

ResourceTypeID Editor::extension_to_tid(KStr ext) {
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" ||
        ext == ".exr" || ext == ".hdr")
        return type_id<Texture>();
    if (ext == ".world") return type_id<WorldSetting>();
    // if (ext == ".terrain")  // adjust to your format
    //     return type_id<Terrain>();
    return 0;
}

void Editor::scan_assets() {
    Project *project = SeedEngine::get_instance()->get_project();
    Ref<Dir> dir = Dir::open(project->get_asset_dir());
    std::queue<Ref<Dir>> dirs_to_process;
    dirs_to_process.push(dir);
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    while (!dirs_to_process.empty()) {
        Ref<Dir> d = dirs_to_process.front();
        dirs_to_process.pop();
        std::vector<Path> childs = d->list();
        for (Path &path : childs) {
            path = d->concat(path);

            if (path.is_directory()) {
                dirs_to_process.push(Dir::open(path));
            } else {
                path = path.relative(project->get_path());
                if (!entries.get_uuid(path).is_null()) continue;
                KStr extension = path.extension();
                u64 tid = extension_to_tid(extension);
                if (tid == 0) continue;
                entries.insert_entry(path, tid);
            }
        }
    }
    entries.save(project->get_entry_path());
}
ResourceEntry *Editor::create_asset(KStr name, ResourceTypeID tid) {
    Project *project = SeedEngine::get_instance()->get_project();

    Path path = "assets";
    path.push(name);
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    UUID uuid = entries.insert_entry(path, tid);
    entries.save(project->get_entry_path());
    return entries.get_entry(uuid);
}

ResourceEntry *Editor::create_internal_asset(KStr name, ResourceTypeID tid) {
    Project *project = SeedEngine::get_instance()->get_project();

    Path path = "assets/.internal";
    path.push(name);
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    UUID uuid = entries.insert_entry(path, tid);
    entries.save(project->get_entry_path());
    return entries.get_entry(uuid);
}

void Editor::remove_asset(UUID uuid) {
    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    ResourceEntry *entry = entries.get_entry(uuid);
    if (!entry) return;
    File::remove(entry->real_path());
    entries.remove_entry(uuid);
}

void Editor::import_asset(const Path &origin_path, const Path &target_dir) {
    Project *project = SeedEngine::get_instance()->get_project();

    ResourceEntries &entries = ResourceLoader::get_instance()->get_entries();
    Path moved_path = target_dir.append(origin_path.filename());
    Ref<File> origin = File::open(origin_path);
    origin->copy_to(project->get_path().append(moved_path));

    bool r = gEditor->preprocessor.try_preprocess(entries, origin, moved_path);
    if (r) {
        gEditor->preprocessor.get_entries().save(
            project->get_preprocess_entry_path());
    } else {
        KStr extension = origin_path.extension();
        u64 tid = extension_to_tid(extension);
        if (tid == 0) return;
        entries.insert_entry(moved_path, tid);
    }
    entries.save(project->get_entry_path());
}

void Editor::save_project() {
    Project *project = SeedEngine::get_instance()->get_project();
    project->save();
}

}  // namespace Seed

using namespace Seed;
int main(int, char **) {
    NFD_Init();
    // Main loop
    Seed::SeedEngine *engine = new Seed::SeedEngine(60.0f);
    RenderEngine *render_engine = RenderEngine::get_instance();
    Editor *editor = new Editor;

    auto &ecs = engine->get_world()->ecs();
    EditorCameraEntity::create_entity(ecs);
    ResourceLoader *loader = ResourceLoader::get_instance();
    render_engine->set_renderer_enable(render_engine->get_default_renderer(),
                                       false);

    engine->start();
    NFD_Quit();
    return 0;
}
// Main code
