#include "material.h"

namespace Seed {
Material::~Material() {
    this->diffuse_map.dealloc();
    this->specular_map.dealloc();
    this->normal_map.dealloc();
}
Ref<Material> Material::create(
                               RenderResource diffuse_map,
                               RenderResource specular_map, RenderResource normal_map, f32 shiness) {
    Ref<Material> material;
    material.create();
    material->diffuse_map = diffuse_map;
    material->specular_map = specular_map;
    material->normal_map = normal_map;
    material->shiness = shiness;
    return material;
}
}  // namespace Seed
