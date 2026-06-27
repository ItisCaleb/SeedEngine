#ifndef _SEED_RESOURCE_LOADER_H_
#define _SEED_RESOURCE_LOADER_H_

#include <spdlog/spdlog.h>
#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include "core/container/kstring.h"
#include "core/engine.h"
#include "core/io/file.h"
#include "core/io/path.h"
#include "core/misc/uuid.h"
#include "core/ref.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"
#include "core/concurrency/thread_pool.h"
#include "resource.h"
#include "resource_entry.h"
#include "core/misc/type_name.h"

namespace Seed {
class ResourceLoader;

template <typename T>
class AsyncResource : public RefCounted {
        friend class ResourceLoader;

    private:
        WorkId work_id;
        Ref<T> resource;
        bool loaded = false;

    public:
        Ref<T> wait() {
            if (this->loaded) {
                return this->resource;
            }
            ThreadPool::get_instance()->wait(this->work_id);
            return this->resource;
        }
        bool is_loaded() { return loaded; }
};
#define RESOURCE_LOADER(name)                                                 \
    Ref<Resource> name(ResourceLoader &loader, ResourceConfiguration &config, \
                       Ref<File> data);

class ResourceLoader {
    private:
        inline static ResourceLoader *instance = nullptr;
        std::unordered_map<UUID, Resource *> res_cache;
        std::unordered_map<ResourceTypeID, ResourceTypeInfo> infos;
        std::unordered_map<ResourceTypeID, Ref<Resource>> default_resources;
        static RESOURCE_LOADER(load_shader);
        static RESOURCE_LOADER(load_basic_model);
        static RESOURCE_LOADER(load_skeleton_model);
        static RESOURCE_LOADER(load_texture);
        static RESOURCE_LOADER(load_mappable_texture);
        static RESOURCE_LOADER(load_world);
        static void load_meshes(ResourceLoader &loader,
                                ResourceConfiguration &config, Ref<File> data,
                                std::vector<Ref<Mesh>> &meshes,
                                Ref<Skeleton> skeleton,
                                std::vector<Ref<Animation>> &animations);
        ResourceEntries entries;

    public:
        static ResourceLoader *get_instance();
        void register_resource(Resource *res);
        void unregister_resource(Resource *res);
        ResourceEntries &get_entries() { return entries; }

        RHI::UpdateBufferInfo load_image_to_upload(UUID uuid,
                                                   bool force_rgba = false);
        Ref<Image> load_image(UUID uuid, bool force_rgba = false);

        template <typename T>
        Ref<T> load(UUID uuid) {
            static_assert(std::is_base_of_v<Resource, T>);
            if (res_cache.find(uuid) != res_cache.end()) {
                return Ref<T>(static_cast<T *>(res_cache[uuid]));
            }
            ResourceEntry *entry = entries.get_entry(uuid);
            if (entry == nullptr) {
                return Ref<T>();
            }
            u64 tid = type_id<T>();
            auto iter = infos.find(tid);
            if (iter == infos.end()) {
                return Ref<T>();
            }
            Ref<Resource> res;

            const Path path =
                SeedEngine::get_instance()->get_project()->resolve_asset(
                    entry->path);
            Ref<File> file = File::open(path);
            if (file.is_null()) {
                return Ref<T>();
            }
            if (iter->second.has_data) {
                res = iter->second.load(*this, entry->config, file);
            } else {
                ResourceConfiguration config(file->read_json());
                res = iter->second.load(*this, config, file);
            }
            if (res.is_null()) {
                return ref_cast<T>(res);
            }
            res->set_uuid(uuid);
            this->register_resource(res.ptr());
            return ref_cast<T>(res);
        }

        template <typename T>
        Ref<T> load_from_path(const Path &path) {
            static_assert(std::is_base_of_v<Resource, T>);
            UUID uuid = entries.get_uuid(path);
            if (uuid.is_null()) {
                return Ref<T>();
            }
            return load<T>(uuid);
        }

        template <typename T>
        Ref<T> load(ResourceConfiguration &config, KStr key) {
            static_assert(std::is_base_of_v<Resource, T>);
            nlohmann::ordered_json &j = config.get_json();
            UUID uuid = config.get<UUID>(key);
            return load<T>(uuid);
        }

        template <typename T>
        Ref<T> load_or_default() {}

        template <typename T>
        Ref<T> load_internal(const Path &path) {
            /* internal load */
            static_assert(std::is_base_of_v<Resource, T>);
            u64 tid = type_id<T>();
            UUID uuid = entries.insert_entry(path, tid, true);
            auto iter = infos.find(tid);
            if (iter == infos.end()) {
                return Ref<T>();
            }
            ResourceEntry *entry = entries.get_entry(uuid);
            Ref<Resource> res;
            Ref<File> file = File::open(entry->path);
            if (file.is_null()) {
                return Ref<T>();
            }
            if (iter->second.has_data) {
                res = iter->second.load(*this, entry->config, file);
            } else {
                ResourceConfiguration config(file->read_json());
                res = iter->second.load(*this, config, file);
            }

            if (res.is_null()) {
                return ref_cast<T>(res);
            }
            res->set_uuid(uuid);
            return ref_cast<T>(res);
        }

        template <typename T>
        Ref<AsyncResource<T>> load_async(
            UUID uuid, std::function<void(Ref<T>)> callback = {}) {
            Ref<AsyncResource<T>> async_rc;
            async_rc.create();
            async_rc->work_id = ThreadPool::get_instance()->add_work(
                [=](void *) mutable {
                    async_rc->resource = load<T>(uuid);
                    async_rc->loaded = true;
                    if (callback) {
                        callback(async_rc->resource);
                    }
                },
                nullptr);
            return async_rc;
        }
        template <typename T>
        Ref<AsyncResource<T>> load_async_from_path(
            const Path &path, std::function<void(Ref<T>)> callback = {}) {
            Ref<AsyncResource<T>> async_rc;
            async_rc.create();
            async_rc->work_id = ThreadPool::get_instance()->add_work(
                [=](void *) mutable {
                    async_rc->resource = load_from_path<T>(path);
                    async_rc->loaded = true;
                    if (callback) {
                        callback(async_rc->resource);
                    }
                },
                nullptr);
            return async_rc;
        }

        template <typename T>
        void register_type(std::function<Ref<Resource>(
                               ResourceLoader &, ResourceConfiguration &config,
                               Ref<File> data)>
                               load_func,
                           bool has_data = false) {
            ResourceTypeID id = type_id<T>();
            this->infos[id] = ResourceTypeInfo{
                .id = id, .has_data = has_data, .load = load_func};
        }

        template <typename T>
        void register_type_config(
            std::function<void(ResourceConfiguration &)> config_func,
            bool has_data = false) {
            ResourceTypeID tid = type_id<T>();
            this->infos[tid].generate_config = config_func;
        }

        template <typename T>
        void register_default_resource(Ref<T> default_resource) {
            default_resources[type_id<T>()] =
                ref_cast<Resource>(default_resource);
        }

        ResourceTypeInfo *get_type_info(ResourceTypeID tid) {
            auto iter = infos.find(tid);
            if (iter == infos.end()) {
                return nullptr;
            }
            return &iter->second;
        }
        Ref<TextureCubemap> load_cubemap(u32 w, u32 h, UUID right, UUID left,
                                         UUID top, UUID bottom, UUID front,
                                         UUID back);
        ResourceLoader(/* args */);
        ~ResourceLoader();
};
}  // namespace Seed
#endif