#include "material.h"

namespace Seed {
Material::~Material() {
    this->texture_rc.dealloc();
    this->diffuse_map.dealloc();
    this->specular_map.dealloc();
}
Ref<Material> Material::create(RenderResource texture,
                               RenderResource diffuse_map,
                               RenderResource specular_map, f32 shiness) {
    Ref<Material> material;
    material.create();
    material->texture_rc = texture;
    material->diffuse_map = diffuse_map;
    material->specular_map = specular_map;
    material->shiness = shiness;
    return material;
}
}  // namespace Seed
