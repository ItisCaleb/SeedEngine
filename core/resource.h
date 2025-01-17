#ifndef _SEED_RESOURCE_H_
#define _SEED_RESOURCE_H_
#include "ref.h"
#include "rendering/mesh.h"
#include "rendering/texture.h"
#include "rendering/api/render_resource.h"
#include "types.h"

#include <string>

namespace Seed {
class ResourceLoader {
   private:
    inline static ResourceLoader *instance = nullptr;

   public:
    static ResourceLoader *get_instance();
    template <typename T>
    Ref<T> load(const std::string &path);

    RenderResource load_texture(const std::string &path);
    RenderResource loadShader(const std::string &vertex_path,
                              const std::string &fragment_path);

    ResourceLoader(/* args */);
    ~ResourceLoader();
};

}  // namespace Seed

#endif