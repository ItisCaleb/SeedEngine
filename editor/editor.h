#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_
#include <string>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/resource/resource.h"
#include "editor_gui.h"
#include "gui/inspectable.h"
#include "gui/popup.h"
#include "project/preprocessor.h"
#include "project/project.h"
#include <nlohmann/json.hpp>
#include "editor/world/terrain_editor.h"
#include "editor/asset/asset_viewer.h"
#include "editor/asset/asset_browser.h"

namespace Seed {
class Editor {
        friend EditorGUI;
        Project *current_project = nullptr;
        nlohmann::json project_cache;
        void set_last_open(Path &path);

    public:
        struct Context {
                Inspectable *current_inspect = nullptr;
                Popup *current_popup = nullptr;
        } ctx;
        TerrainEditor terrain_editor;
        AssetViewer asset_viewer;
        AssetBrowser asset_browser;
        Preprocessor preprocessor;
        EditorGUI editor_gui;
        Inspector inspector;
        void set_last_open_world(const Path &path);
        void set_current_inspect(Inspectable *inspectable);
        void set_current_popup(Popup *popup);
        Project *project() { return current_project; }

        Editor();
        ~Editor() = default;
};

extern Editor *gEditor;

}  // namespace Seed

#endif