#ifndef _SEED_RML_INTERFACE_H_
#define _SEED_RML_INTERFACE_H_

#include "core/math/vec2.h"
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/texture.h"
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/SystemInterface.h>
#include <unordered_map>
#include <vector>

namespace Seed {

struct RmlVertex {
        Vec2 pos;
        Vec2 uv;
        Color color;
};

struct RmlGeometry {
        VertexHandle vertex;
        IndexHandle index;
        u32 vertex_count = 0;
        u32 index_count = 0;
};

struct RmlDrawCommand {
        Rml::CompiledGeometryHandle geometry;
        Rml::Vector2f translation;
        Rml::Matrix4f transform;
        Rml::TextureHandle texture;
        bool scissor_enabled;
        bool clip_enabled;
        Rml::Rectanglei scissor;
};

class SeedRmlRenderInterface : public Rml::RenderInterface {
    private:
        std::unordered_map<Rml::CompiledGeometryHandle, RmlGeometry> geometries;
        std::unordered_map<Rml::TextureHandle, Ref<Texture>> textures;
        std::vector<RmlDrawCommand> commands;
        Rml::CompiledGeometryHandle next_geometry = 1;
        Rml::TextureHandle next_texture = 1;
        bool clip_enabled;
        bool scissor_enabled = false;
        Rml::Rectanglei scissor;
        Rml::RowMajorMatrix4f transform = Rml::RowMajorMatrix4f::Identity();

        Rml::TextureHandle store_texture(Ref<Texture> texture);

    public:
        const std::vector<RmlDrawCommand> &get_commands() const {
            return commands;
        }
        const RmlGeometry *get_geometry(
            Rml::CompiledGeometryHandle geometry) const;
        TextureHandle get_texture(Rml::TextureHandle texture) const;
        void begin_frame();

        Rml::CompiledGeometryHandle CompileGeometry(
            Rml::Span<const Rml::Vertex> vertices,
            Rml::Span<const int> indices) override;
        void RenderGeometry(Rml::CompiledGeometryHandle geometry,
                            Rml::Vector2f translation,
                            Rml::TextureHandle texture) override;
        void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;
        Rml::TextureHandle LoadTexture(Rml::Vector2i &texture_dimensions,
                                       const Rml::String &source) override;
        Rml::TextureHandle GenerateTexture(
            Rml::Span<const Rml::byte> source,
            Rml::Vector2i source_dimensions) override;
        void ReleaseTexture(Rml::TextureHandle texture) override;
        void EnableScissorRegion(bool enable) override;
        void SetScissorRegion(Rml::Rectanglei region) override;
        void SetTransform(const Rml::Matrix4f *transform) override;

        SeedRmlRenderInterface();
        ~SeedRmlRenderInterface();
};

class SeedRmlElementInstancer : public Rml::ElementInstancer {
    public:
        void RegisterElements();
        // Instances an element given the tag name and attributes.
        // @param[in] parent The element the new element is destined to be
        // parented to.
        // @param[in] tag The tag of the element to instance.
        // @param[in] attributes Dictionary of attributes.
        // @return A unique pointer to the instanced element.
        Rml::ElementPtr InstanceElement(
            Rml::Element *parent, const Rml::String &tag,
            const Rml::XMLAttributes &attributes) override;

        // Releases an element instanced by this instancer.
        // @param[in] element The element to release.
        void ReleaseElement(Rml::Element *element) override;
};

class SeedRmlSystemInterface : public Rml::SystemInterface {
    public:
        double GetElapsedTime() override;

        bool LogMessage(Rml::Log::Type type,
                        const Rml::String &message) override;
};

}  // namespace Seed

#endif