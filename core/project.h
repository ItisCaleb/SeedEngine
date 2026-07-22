#ifndef _SEED_PROJECT_H_
#define _SEED_PROJECT_H_
#include <string>
#include "core/io/path.h"

namespace Seed {

class Project {
    private:
        std::string name;
        Path path;
        Path asset_dir;
        Path internal_dir;
        Path entry_path;
        Path preprocess_entry_path;

    public:
        static Project *load(const Path &path);
        Path &get_asset_dir() { return asset_dir; }
        Path &get_internal_dir() { return internal_dir; }
        const Path resolve_asset(const Path &path);
        const Path &get_entry_path() { return entry_path; }
        const Path &get_preprocess_entry_path() {
            return preprocess_entry_path;
        }
        const Path &get_path() { return path; }
        void save();
};
}  // namespace Seed

#endif
