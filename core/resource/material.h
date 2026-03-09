#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include "core/math/vec3.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/texture.h"
#include "core/resource/resource.h"
#include "core/resource/shader.h"
#include "core/resource/default_storage.h"

namespace Seed {

class RenderDrawDataBuilder;
class Material : public Resource {
    protected:
        u16 id;
        std::unordered_map<u32, Ref<Texture>> textures;
        PipelineHandle pipeline = NULL_HANDLE;
        Ref<Shader> shader;
        RenderRasterizerState raster_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_state;

    public:
        void set_texture_unit(u32 unit, Ref<Texture> texture);
        void set_texture(const std::string &name, Ref<Texture> texture);
        void remove_texture_unit(u32 unit);
        void remove_texture(const std::string &name);

        Ref<Texture> get_texture_unit(u32 unit);
        Ref<Texture> get_texture(const std::string &name);

        u32 get_texture_count() { return textures.size(); }

        RenderRasterizerState get_rasterizer_state() { return raster_state; }
        RenderDepthStencilState get_depth_state() { return depth_state; }
        RenderBlendState get_blend_state() { return blend_state; }

        PipelineHandle get_pipeline();
        u16 get_id() { return id; }
        virtual void bind_states(RenderDrawDataBuilder &builder);
        inline static u16 last_id = 0;
        Material(Ref<Shader> shader) : id(last_id++), shader(shader) {}
        Material(Ref<Shader> shader, const RenderRasterizerState &rst_state,
                 const RenderDepthStencilState &depth_state,
                 const RenderBlendState &blend_state)
            : id(last_id++),
              shader(shader),
              raster_state(rst_state),
              depth_state(depth_state),
              blend_state(blend_state) {}

        ~Material() {}
};

class BaseMaterial : public Material {
    private:
        inline static const char *name_map[] = {"u_diffuse", "u_specular",
                                                "u_normal"};

    public:
        enum TextureMapType : u8 { DIFFUSE = 0, SPECULAR, NORMAl, MAX };

        f32 shiness;
        BaseMaterial() : BaseMaterial(DS::get_instance()->mesh_shader) {}

        BaseMaterial(Ref<Shader> shader) : Material(shader) {
            this->set_texture(name_map[DIFFUSE],
                              DS::get_instance()->white_texture);
            this->set_texture(name_map[SPECULAR],
                              DS::get_instance()->white_texture);
            this->set_texture(name_map[NORMAl],
                              DS::get_instance()->white_texture);
            depth_state = {.depth_mode = DepthMode::OPAQUE};
        }
        void set_texture_map(TextureMapType type, Ref<Texture> tex) {
            this->set_texture(name_map[type], tex);
        }
};

}  // namespace Seed

#endif