#ifndef _SEED_RESOURCE_ENTRY_H_
#define _SEED_RESOURCE_ENTRY_H_

#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <unordered_set>
#include "core/container/kstring.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/resource/resource.h"
#include "resource.h"

namespace Seed {

class ResourceConfiguration {
    private:
        nlohmann::ordered_json config = nlohmann::json::object();

    public:
        ResourceConfiguration() = default;
        ResourceConfiguration(const nlohmann::json &config) : config(config) {}
        template <typename T>
        const T &get(KStr key);
        template <typename T>
        const void put(KStr key, const T &value);

        template <typename T>
        void deserialize(T &v);
        template <typename T>
        void sereialize(const T &v);
        nlohmann::ordered_json &get_json() { return config; }
};

struct ResourceEntry {
        UUID uuid;
        ResourceTypeID type_id;
        /* if resource has data e.g. bin file, image data */
        /* then the path will point to it */
        /* else it is the config path */
        Path path;
        ResourceConfiguration config;
        Path real_path();
};

class ResourceEntries {
    private:
        /* use map here to prevent entry rearrange every time */
        std::map<UUID, ResourceEntry> uuid_to_entry;
        std::unordered_map<Path, UUID> path_to_uuid;
        std::unordered_set<UUID> internal_entries;

    public:
        ResourceEntry *get_entry(const UUID uuid);
        UUID get_uuid(const Path &path);
        UUID insert_entry(const Path &p, ResourceTypeID id,
                          bool is_internal = false);
        void remove_entry(const UUID uuid);
        void save(const Path &path);
        void load(const Path &path);
};

}  // namespace Seed

#endif