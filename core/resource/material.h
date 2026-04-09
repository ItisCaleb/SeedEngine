#ifndef _SEED_MATERIAL_H_
#define _SEED_MATERIAL_H_

#include <string>
#include "core/rendering/rhi/render_command.h"
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
        i16 shadow_map_unit = -1;
        std::unordered_map<u32, Ref<Texture>> textures;
        PipelineHandle pipeline = NULL_HANDLE;
        Ref<Shader> shader;
        RenderRasterizerState raster_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_state;
        bool cast_shadow = true;
        bool receive_shadow = true;
        /* store material parameters*/
        void *parameters = nullptr;
        u32 param_size;

    public:
        void set_texture_unit(u32 unit, Ref<Texture> texture);
        void set_texture(const std::string &name, Ref<Texture> texture);
        void remove_texture_unit(u32 unit);
        void remove_texture(const std::string &name);

        void set_parameter(const std::string &name, const void *value, u32 size);

        template <typename T>
        void set_parameter(const std::string &name, const T &value) {
            set_parameter(name, &value, sizeof(T));
        }

        i16 get_shadow_map_unit() { return shadow_map_unit; }

        Ref<Texture> get_texture_unit(u32 unit);
        Ref<Texture> get_texture(const std::string &name);

        u32 get_texture_count() { return textures.size(); }

        RenderRasterizerState get_rasterizer_state() { return raster_state; }
        RenderDepthStencilState get_depth_state() { return depth_state; }
        RenderBlendState get_blend_state() { return blend_state; }

        bool do_cast_shadow() { return cast_shadow; }
        bool do_receive_shadow() { return receive_shadow; }

        PipelineHandle get_pipeline();
        u16 get_id() { return id; }
        void upload_parameter(RenderCommandDispatcher &dp);
        virtual void bind_states(RenderDrawDataBuilder &builder);
        inline static u16 last_id = 0;
        Material(Ref<Shader> shader, const RenderRasterizerState &rst_state,
                 const RenderDepthStencilState &depth_state,
                 const RenderBlendState &blend_state);
        Material(Ref<Shader> shader) : Material(shader, {}, {}, {}) {}

        ~Material() {
            if (parameters != nullptr) {
                free(parameters);
            }
        }
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
                              DS::get_instance()->normal_texture);
            depth_state = {.depth_mode = DepthMode::OPAQUE};
        }
        void set_texture_map(TextureMapType type, Ref<Texture> tex) {
            this->set_texture(name_map[type], tex);
        }
};

}  // namespace Seed

#endif