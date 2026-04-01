#ifndef _SEED_PROJECT_H_
#define _SEED_PROJECT_H_
#include <list>
#include <string>
#include <map>
#include "core/world/world.h"

namespace Seed {
class EditorGUI;
class Project {
        friend EditorGUI;

    private:
        std::string name;
        std::string path;
        std::list<std::string> assets;
        void save();

    public:
        static Project *load(const std::string &path);
        std::string get_asset_dir();
        void add_to_assets(const std::string &path) {
            this->assets.push_back(path);
        }
};
}  // namespace Seed

#endif