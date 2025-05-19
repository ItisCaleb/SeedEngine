#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"
#include "core/rendering/api/render_resource.h"
#include "core/resource/texture.h"
#include "core/resource/resource.h"
#include "core/resource/shader.h"
#include "core/resource/default_storage.h"

namespace Seed {
class TextureState {
    private:
        Ref<Texture> texture;

    public:
        TextureState(Ref<Texture> texture) : texture(texture) {}
        void bind_texture(Ref<Texture> texture) { this->texture = texture; }
        Ref<Texture> get_texture() { return texture; }
        ~TextureState() = default;
};
class RenderDrawDataBuilder;
class Material : public Resource {
    protected:
        u16 id;
        std::vector<TextureState> textures;
        RenderResource pipeline;
        bool pipeline_dirty = true;
        Ref<Shader> shader;
        RenderRasterizerState raster_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_state;

    public:
        void build_pipeline();
        void set_texture_unit(u32 unit, Ref<Texture> texture);
        void add_texture_unit(Ref<Texture> tex);
        void remove_texture_unit(u32 unit);
        TextureState *get_texture_unit(u32 unit);
        void set_rasterizer_state(RenderRasterizerState &state);
        void set_depth_state(RenderDepthStencilState &state);
        void set_blend_state(RenderBlendState &state);
        void set_shader(Ref<Shader> shader);
        RenderRasterizerState get_rasterizer_state(){
            return raster_state;
        }
        RenderDepthStencilState get_depth_state(){
            return depth_state;
        }
        RenderBlendState get_blend_state(){
            return blend_state;
        }

        RenderResource get_pipeline();
        u16 get_id() { return id; }
        virtual void bind_states(RenderDrawDataBuilder &builder);
        inline static u16 last_id = 0;
        Material(Ref<Shader> shader) : id(last_id++), shader(shader) {}
        ~Material() {}
};

class BaseMaterial : public Material {
    public:
        enum TextureMapType : u8 { DIFFUSE = 0, SPECULAR, NORMAl, MAX };
        f32 shiness;
        BaseMaterial() : Material(DS::get_instance()->get_mesh_shader()) {
            for (i32 i = 0; i < TextureMapType::MAX; i++) {
                this->add_texture_unit(Ref<Texture>());
            }
            depth_state = {.depth_on = true};
        }
        void set_texture_map(TextureMapType type, Ref<Texture> tex) {
            this->set_texture_unit(type, tex);
        }
};

}  // namespace Seed

#endif