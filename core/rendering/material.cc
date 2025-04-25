#include "material.h"
#include <spdlog/spdlog.h>

namespace Seed {

void Material::set_texture_map(TextureMapType type, Ref<Texture> texture){
    if(type >= TextureMapType::MAX){
        SPDLOG_WARN("TextureMapType to big.");
        return;
    }
    this->textures[type] = texture;
}
Ref<Texture> Material::get_texture_map(TextureMapType type){
    if(type >= TextureMapType::MAX){
        SPDLOG_WARN("TextureMapType to big.");
        return Ref<Texture>();
    }
    return this->textures[type];
}

}  // namespace Seed
