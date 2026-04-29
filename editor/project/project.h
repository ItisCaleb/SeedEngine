#ifndef _SEED_PROJECT_H_
#define _SEED_PROJECT_H_
#include <list>
#include <string>
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/resource/resource.h"

namespace Seed {
class EditorGUI;

class Project {
        friend EditorGUI;

    private:
        std::string name;
        Path path;
        Path asset_dir;
        Path internal_dir;
        Path entry_path;
        Path preprocess_entry_path;
        std::list<std::string> assets;
        void save();

    public:
        static Project *load(const std::string &path);
        Path &get_asset_dir() { return asset_dir; }
        Path &get_internal_dir() { return internal_dir; }
        void add_to_assets(const std::string &path) {
            this->assets.push_back(path);
        }
        void scan_assets();
        UUID create_asset(const Path &path, ResourceTypeID tid);
        void import_asset(const Path &origin_path, const Path &target_dir);
        ResourceTypeID extension_to_tid(KStr ext);

        const Path &get_entry_path() { return entry_path; }
        const Path &get_preprocess_entry_path() {
            return preprocess_entry_path;
        }
        const Path &get_path() { return path; }
};
}  // namespace Seed

#endif