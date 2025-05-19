#ifndef _SEED_RESOURCE_LOADER_H_
#define _SEED_RESOURCE_LOADER_H_

#include "core/ref.h"
#include "core/resource/resource.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/shader.h"
#include "core/concurrency/thread_pool.h"

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
class ResourceLoader {
    private:
        inline static ResourceLoader *instance = nullptr;
        template <typename T>
        Ref<T> _load(const std::string &path);
        std::unordered_map<std::string, Resource *> res_cache;

    public:
        static ResourceLoader *get_instance();
        void register_resource(Resource *res);
        void unregister_resource(Resource *res);
        template <typename T>
        Ref<T> load(const std::string &path) {
            static_assert(std::is_base_of_v<Resource, T>);
            if (res_cache.find(path) != res_cache.end()) {
                return Ref<T>(static_cast<T *>(res_cache[path]));
            }
            Ref<Resource> res = ref_cast<Resource>(this->_load<T>(path));
            res->set_path(path);
            this->register_resource(res.ptr());
            return ref_cast<T>(res);
        }

        template <typename T>
        Ref<AsyncResource<T>> load_async(
            const std::string &path,
            std::function<void(Ref<T>)> callback = {}) {
            Ref<AsyncResource<T>> async_rc;
            async_rc.create();
            async_rc->work_id = ThreadPool::get_instance()->add_work(
                [=](void *) mutable {
                    async_rc->resource = _load<T>(path);
                    async_rc->loaded = true;
                    if (callback) {
                        callback(async_rc->resource);
                    }
                },
                nullptr);
            return async_rc;
        }

        Ref<Shader> load_shader(const std::string &vertex_path,
                                const std::string &fragment_path,
                                const std::string &geometry_path = "",
                                const std::string &tess_ctrl_path = "",
                                const std::string &tess_eval_path = "");

        ResourceLoader(/* args */);
        ~ResourceLoader();
};
}  // namespace Seed
#endif