#ifndef _SEED_PREPROCESSOR_H_
#define _SEED_PREPROCESSOR_H_

#include <unordered_map>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/misc/type_name.h"
#include "core/misc/uuid.h"
#include "core/resource/resource.h"
#include "core/resource/resource_entry.h"
namespace Seed {
struct PreprocessedResult {
        ResourceTypeID target_tid;
        Path out_file;
};

struct PreprocessTypeInfo {
        ResourceTypeID id;
        std::function<bool(ResourceConfiguration &in_conf, Ref<File> &in_data,
                           ResourceConfiguration &out_conf, Ref<Dir> out_dir,
                           PreprocessedResult &out_result)>
            preprocess;
        std::function<void(ResourceConfiguration &)> generate_config;
};

struct PreprocessEntry {
        UUID uuid;
        UUID target_uuid;
        ResourceTypeID type_id;
        Path path;
        ResourceConfiguration config;
};

struct PreprocessEntries {
    private:
        std::unordered_map<UUID, PreprocessEntry> uuid_to_entry;
        std::unordered_map<Path, UUID> path_to_uuid;

    public:
        PreprocessEntry *get_entry(const UUID uuid);
        UUID get_uuid(const Path &path);
        UUID insert_entry(const Path &p, ResourceTypeID id);
        void link_entry(UUID from, UUID to);

        void remove_entry(const UUID uuid);
        void save(const Path &path);
        void load(const Path &path);
};

class Preprocessor {
    private:
        Path preprocess_dir;
        PreprocessEntries preprocess_entries;
        std::unordered_map<ResourceTypeID, PreprocessTypeInfo> infos;
        std::unordered_map<KString, PreprocessTypeInfo> ext_to_infos;

        bool preprocess(ResourceEntries &entries, Ref<File> file,
                        const Path &moved_path, PreprocessTypeInfo &info);
        static bool process_model(ResourceConfiguration &in_conf,
                                  Ref<File> &in_data,
                                  ResourceConfiguration &out_conf,
                                  Ref<Dir> out_dir,
                                  PreprocessedResult &out_result);

    public:
        bool try_preprocess(ResourceEntries &entries, Ref<File> file,
                            const Path &moved_path);

        template <typename T>
        void register_type(
            const std::vector<KString> &extensions,
            std::function<
                bool(ResourceConfiguration &in_conf, Ref<File> &in_data,
                     ResourceConfiguration &out_conf, Ref<Dir> out_dir,
                     PreprocessedResult &out_result)>
                preprocess_func) {
            ResourceTypeID id = type_id<T>();
            this->infos[id] =
                PreprocessTypeInfo{.id = id, .preprocess = preprocess_func};
            for (auto &ext : extensions) {
                this->ext_to_infos[ext] =
                    PreprocessTypeInfo{.id = id, .preprocess = preprocess_func};
            }
        }

        template <typename T>
        void register_type_config(
            std::function<bool(ResourceConfiguration &)> config_func) {
            ResourceTypeID tid = type_id<T>();
            this->infos[tid].generate_config = config_func;
        }

        void init(const Path &path);
        PreprocessEntry *get_entry_from_path(const Path &path) {
            UUID uuid = this->preprocess_entries.get_uuid(path);
            if (uuid.is_null()) return nullptr;
            return this->preprocess_entries.get_entry(uuid);
        }

        PreprocessEntries &get_entries() { return preprocess_entries; }
        Preprocessor();
};
}  // namespace Seed

#endif