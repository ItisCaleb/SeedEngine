#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_
#include <string>
#include "core/io/path.h"
#include "project/project.h"
#include <nlohmann/json.hpp>
#include "editor/world/terrain_editor.h"
#include "editor/asset/asset_viewer.h"
#include "editor/asset/asset_browser.h"

namespace Seed {
class EditorGUI;
class Editor {
        friend EditorGUI;
        Project *current_project = nullptr;
        nlohmann::json project_cache;
        void set_last_open(Path &path);

    public:
        struct Context {
        } ctx;
        TerrainEditor terrain_editor;
        AssetViewer asset_viewer;
        AssetBrowser asset_browser;
        void set_last_open_world(const Path &path);
        Project *project() { return current_project; }

        Editor();
        ~Editor() = default;
};

extern Editor *gEditor;

}  // namespace Seed

#endif