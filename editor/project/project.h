#ifndef _SEED_PROJECT_H_
#define _SEED_PROJECT_H_

#include <string>

#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/ref.h"
#include "core/resource/resource.h"
#include "editor/project/preprocessor.h"

namespace Seed {

class Project : public RefCounted {
    private:
        std::string name;
        Path path;
        Path asset_dir;
        Path internal_dir;
        Path entry_path;
        Path preprocess_entry_path;
        Preprocessor preprocessor;

        ResourceTypeID extension_to_type(KStr extension) const;

    public:
        Project() = default;
        static Ref<Project> load(const Path &path);

        const Path &get_asset_dir() const { return asset_dir; }
        const Path &get_internal_dir() const { return internal_dir; }
        Path resolve_asset(const Path &path) const;
        const Path &get_entry_path() const { return entry_path; }
        const Path &get_preprocess_entry_path() const {
            return preprocess_entry_path;
        }
        const Path &get_path() const { return path; }

        void scan_assets();
        ResourceEntry *create_asset(const Path &path, ResourceTypeID type_id);
        ResourceEntry *create_internal_asset(KStr name, ResourceTypeID type_id);
        void remove_asset(UUID uuid);
        void import_asset(const Path &origin_path, const Path &target_dir);
        PreprocessEntry *get_preprocessed_entry(const Path &path);
        void save();
};
}  // namespace Seed

#endif
