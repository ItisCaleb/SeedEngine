#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_
#include <string>
#include "project/project.h"
#include <nlohmann/json.hpp>
#include "editor_terrain.h"

namespace Seed {
class EditorGUI;
class Editor {
        friend EditorGUI;
        inline static Editor *instance = nullptr;
        Project *current_project = nullptr;
        nlohmann::json project_cache;
        void set_last_open(std::string &path);

    public:
        static Editor* get_instance(){
            return instance;
        }
        Ref<EditorTerrain> terrain;

        Editor();
        ~Editor() = default;
};
}  // namespace Seed

#endif