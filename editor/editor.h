#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_
#include <string>
#include "project/project.h"
#include <nlohmann/json.hpp>
#include "editor/terrain/terrain_editor.h"
#include "editor/asset/asset_viewer.h"

namespace Seed {
class EditorGUI;
class Editor {
        friend EditorGUI;
        Project *current_project = nullptr;
        nlohmann::json project_cache;
        void set_last_open(std::string &path);

    public:
        TerrainEditor terrain_editor;
        AssetViewer asset_viewer;
        Project *project(){
            return current_project;
        }

        Editor();
        ~Editor() = default;
};

extern Editor* gEditor;

}  // namespace Seed

#endif