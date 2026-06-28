#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_
#include <string>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/resource/resource.h"
#include "gui/inspectable.h"
#include "gui/popup.h"
#include "project/preprocessor.h"
#include <nlohmann/json.hpp>
#include "editor/world/world_editor.h"
#include "editor/asset/asset_viewer.h"
#include "editor/asset/asset_browser.h"
#include <RmlUi/Core/DataModelHandle.h>

namespace Seed {
class Editor : public RmlGUI {
        nlohmann::json project_cache;
        void set_last_open(const Path &path);

    public:
        struct Context {
                Inspectable *current_inspect = nullptr;
                Popup *current_popup = nullptr;
        } ctx;
        WorldEditor world_editor;
        AssetViewer asset_viewer;
        AssetBrowser asset_browser;
        Preprocessor preprocessor;
        Inspector inspector;
        void set_last_open_world(const UUID uuid);
        void set_current_inspect(Inspectable *inspectable);
        void set_current_popup(Popup *popup);

        ResourceTypeID extension_to_tid(KStr ext);
        void scan_assets();
        ResourceEntry *create_asset(KStr name, ResourceTypeID tid);
        ResourceEntry *create_internal_asset(KStr name, ResourceTypeID tid);
        void remove_asset(UUID uuid);
        void try_open_project();
        void save_project();

        void import_asset(const Path &origin_path, const Path &target_dir);
        Ref<Dir> get_current_dir() { return asset_browser.get_current_dir(); }

        bool show_create = false;
        Rml::DataModelHandle project_model;
        std::string project_name_input;
        std::string project_path_input;
        std::string project_error;

        /* commands */
        void new_project(RML_EVENT_ARGS);
        void load_project(RML_EVENT_ARGS);

        void bind_model(Rml::Context *context) override;

        Editor();
        ~Editor();
};

extern Editor *gEditor;

}  // namespace Seed

#endif
