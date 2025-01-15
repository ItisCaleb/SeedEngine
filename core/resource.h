#ifndef _SEED_RESOURCE_H_
#define _SEED_RESOURCE_H_
#include "ref.h"
#include "rendering/mesh.h"
#include "rendering/shader.h"
#include "rendering/texture.h"
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
    template <>
    Ref<Texture> load(const std::string &path);
    template <>
    Ref<Mesh> load(const std::string &path);

    Ref<Shader> loadShader(const std::string &vertex_path, const std::string &fragment_path);

    ResourceLoader(/* args */);
    ~ResourceLoader();
};

}  // namespace Seed

#endif