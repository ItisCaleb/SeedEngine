#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_

#include <string>

#include <RmlUi/Core/DataModelHandle.h>
#include <nlohmann/json.hpp>

#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/resource/resource.h"
#include "core/resource/world_setting.h"
#include "editor/asset/asset_browser.h"
#include "editor/editor_system.h"
#include "editor/gui/inspector_panel.h"
#include "editor/gui/world_editor_panel.h"
#include "editor/gui/world_viewport.h"
#include "editor/project/project.h"

namespace Seed {

class ResourceEntry;
class World;

class Editor : public RmlGUI {
    private:
        nlohmann::json project_cache;
        Ref<Project> project;
        ResourceEntry *world_entry = nullptr;
        Ref<WorldSetting> world_setting;
        World *world = nullptr;
        WorldViewport world_viewport;
        InspectorPanel inspector_panel;
        WorldEditorPanel world_editor_panel;
        AssetBrowser asset_browser;

        void set_last_open(const Path &path);
        void set_last_open_world(const UUID uuid);
        void close_world();
        bool load_world(ResourceEntry *entry);
        bool open_project(const Path &path);
        void try_open_project();

        bool show_create = false;
        Rml::DataModelHandle project_model;
        std::string project_name_input;
        std::string project_path_input;
        std::string project_error;

        void new_project(RML_EVENT_ARGS);
        void load_project(RML_EVENT_ARGS);

        void bind_model(Rml::Context *context) override;

    public:
        bool open_asset(UUID uuid);
        bool save_world();
        bool has_world() const;

        Ref<Project> get_project() const { return project; }
        ResourceEntry *get_world_entry() const { return world_entry; }
        Ref<WorldSetting> get_world_setting() const { return world_setting; }
        World *get_world() const { return world; }
        WorldViewport &get_world_viewport() { return world_viewport; }
        InspectorPanel &get_inspector_panel() { return inspector_panel; }

        void start();
        Editor();
        ~Editor();
};

extern Editor *System::gEditor;

}  // namespace Seed

#endif
