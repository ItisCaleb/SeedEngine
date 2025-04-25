#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"
#include "api/render_resource.h"
#include "core/rendering/texture.h"
#include "core/ref.h"

namespace Seed {
struct Material : public RefCounted {
public:
    enum TextureMapType{
        DIFFUSE,
        SPECULAR,
        NORMAl,
        MAX
    };
private:
    Ref<Texture> textures[TextureMapType::MAX];

    f32 shiness = 32.0f;

public:
    void set_texture_map(TextureMapType type, Ref<Texture> texture);
    Ref<Texture> get_texture_map(TextureMapType type);
    Material(){}
    ~Material(){}
};

}  // namespace Seed

#endif