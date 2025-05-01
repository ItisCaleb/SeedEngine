#include "material.h"
#include <spdlog/spdlog.h>

namespace Seed {

void Material::set_texture_map(TextureMapType type, Ref<Texture> texture){
    assert(type < TextureMapType::MAX && "Index out of bounds");
    this->textures[type] = texture;
}
Ref<Texture> Material::get_texture_map(TextureMapType type){
    assert(type < TextureMapType::MAX && "Index out of bounds");
    return this->textures[type];
}

}  // namespace Seed
