#ifndef _SEED_RESOURCE_LOADER_H_
#define _SEED_RESOURCE_LOADER_H_

#include "core/ref.h"
#include "core/resource/resource.h"
#include "core/rendering/api/render_resource.h"

namespace Seed{
    class ResourceLoader {
        private:
         inline static ResourceLoader *instance = nullptr;
         template <typename T>
         Ref<T> _load(const std::string &path);
         std::unordered_map<std::string, Ref<Resource>> res_cache;
        public:
         static ResourceLoader *get_instance();
         template <typename T>
         Ref<T> load(const std::string &path){
            static_assert(std::is_base_of_v<Resource, T>);
            Ref<Resource> res = ref_cast<Resource>(this->_load<T>(path));
            res->set_path(path);
            return ref_cast<T>(res);
         }
     
         RenderResource loadShader(const std::string &vertex_path,
                                   const std::string &fragment_path,
                                   const std::string &geometry_path = "",
                                   const std::string &tess_ctrl_path = "",
                                   const std::string &tess_eval_path = "");
     
         ResourceLoader(/* args */);
         ~ResourceLoader();
     };
}
#endif