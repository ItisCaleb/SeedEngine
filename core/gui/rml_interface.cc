#include "rml_interface.h"
#include "core/resource/image.h"
#include "core/resource/resource_loader.h"
#include "core/gui/gui_engine.h"
#include "core/gui/rml_widgets.h"
#include <RmlUi/Core/Factory.h>
#include <RmlUi/Core/URL.h>
#include <GLFW/glfw3.h>

namespace Seed {

static void premultiply_alpha(Ref<Image> image) {
    if (!image.is_valid() || image->get_format() != PixelFormat::RGBA) return;
    u8 *data = image->get_data();
    u32 count = image->get_width() * image->get_height();
    for (u32 i = 0; i < count; i++) {
        u8 *pixel = data + i * 4;
        pixel[0] = (u8)((u32)pixel[0] * pixel[3] / 255);
        pixel[1] = (u8)((u32)pixel[1] * pixel[3] / 255);
        pixel[2] = (u8)((u32)pixel[2] * pixel[3] / 255);
    }
}

SeedRmlRenderInterface::SeedRmlRenderInterface() { instance = this; }

SeedRmlRenderInterface::~SeedRmlRenderInterface() { instance = nullptr; }

void SeedRmlRenderInterface::begin_frame() { commands.clear(); }

const RmlGeometry *SeedRmlRenderInterface::get_geometry(
    Rml::CompiledGeometryHandle geometry) const {
    auto iter = geometries.find(geometry);
    if (iter == geometries.end()) return nullptr;
    return &iter->second;
}

TextureHandle SeedRmlRenderInterface::get_texture(
    Rml::TextureHandle texture) const {
    auto iter = textures.find(texture);
    if (iter == textures.end()) return TextureHandle(NULL_HANDLE);
    return iter->second->get_handle();
}

Rml::TextureHandle SeedRmlRenderInterface::store_texture(Ref<Texture> texture) {
    if (!texture.is_valid()) return 0;
    Rml::TextureHandle handle = next_texture++;
    textures[handle] = texture;
    return handle;
}

Rml::CompiledGeometryHandle SeedRmlRenderInterface::CompileGeometry(
    Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    std::vector<RmlVertex> rml_vertices;
    std::vector<u32> rml_indices;
    rml_vertices.reserve(vertices.size());
    rml_indices.reserve(indices.size());

    for (const Rml::Vertex &vertex : vertices) {
        rml_vertices.push_back(
            RmlVertex{.pos = Vec2{vertex.position.x, vertex.position.y},
                      .uv = Vec2{vertex.tex_coord.x, vertex.tex_coord.y},
                      .color = Color{vertex.colour.red, vertex.colour.green,
                                     vertex.colour.blue, vertex.colour.alpha}});
    }
    for (int index : indices) rml_indices.push_back((u32)index);

    Rml::CompiledGeometryHandle handle = next_geometry++;
    geometries[handle] = RmlGeometry{
        .vertex =
            RHI::alloc_vertex(sizeof(RmlVertex), (u32)rml_vertices.size(),
                              UpdateFrequence::STATIC, rml_vertices.data()),
        .index = RHI::alloc_index(rml_indices, UpdateFrequence::STATIC),
        .vertex_count = (u32)rml_vertices.size(),
        .index_count = (u32)rml_indices.size()};
    return handle;
}

void SeedRmlRenderInterface::RenderGeometry(
    Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation,
    Rml::TextureHandle texture) {
    commands.push_back(RmlDrawCommand{.geometry = geometry,
                                      .translation = translation,
                                      .transform = transform,
                                      .texture = texture,
                                      .scissor_enabled = scissor_enabled,
                                      .scissor = scissor});
}

void SeedRmlRenderInterface::ReleaseGeometry(
    Rml::CompiledGeometryHandle geometry) {
    auto iter = geometries.find(geometry);
    if (iter == geometries.end()) return;
    RHI::dealloc(iter->second.vertex);
    RHI::dealloc(iter->second.index);
    geometries.erase(iter);
}

Rml::TextureHandle SeedRmlRenderInterface::LoadTexture(
    Rml::Vector2i &texture_dimensions, const Rml::String &source) {
    Rml::URL url(source);
    Rml::String protocol = url.GetProtocol();
    if (protocol == "file" || protocol.empty()) {
        Ref<Image> image = Image::load_from_file(Path(url.GetPath()), true);
        if (image.is_null()) return 0;
        premultiply_alpha(image);
        texture_dimensions =
            Rml::Vector2i((int)image->get_width(), (int)image->get_height());
        return store_texture(image->create_texture());
    } else if (protocol == "uuid") {
        UUID uuid = UUID::from_string(url.GetHost());
        if (uuid.is_null()) return 0;
        Ref<Texture> texture =
            ResourceLoader::get_instance()->load<Texture>(uuid);
        if (texture.is_null()) return 0;
        texture_dimensions =
            Rml::Vector2i(texture->get_width(), texture->get_height());
        return store_texture(texture);
    } else if (protocol == "internal") {
        Ref<Texture> texture =
            GuiEngine::get_instance()->get_texture(url.GetHost());
        if (texture.is_null()) return 0;
        texture_dimensions =
            Rml::Vector2i(texture->get_width(), texture->get_height());
        return store_texture(texture);
    }
    return 0;
}

Rml::TextureHandle SeedRmlRenderInterface::GenerateTexture(
    Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
    Ref<Texture> texture;
    texture.create(TextureType::TEXTURE_2D, (u32)source_dimensions.x,
                   (u32)source_dimensions.y, PixelFormat::RGBA,
                   SamplerProperty{}, source.data());
    return store_texture(texture);
}

void SeedRmlRenderInterface::ReleaseTexture(Rml::TextureHandle texture) {
    textures.erase(texture);
}

void SeedRmlRenderInterface::EnableScissorRegion(bool enable) {
    scissor_enabled = enable;
}

void SeedRmlRenderInterface::SetScissorRegion(Rml::Rectanglei region) {
    scissor = region;
}

void SeedRmlRenderInterface::SetTransform(const Rml::Matrix4f *transform) {
    if (!transform) {
        this->transform = Rml::RowMajorMatrix4f::Identity();
    } else {
        /* we use row major */
        this->transform = transform->Transpose();
    }
}

void SeedRmlElementInstancer::RegisterElements() {
    Rml::Factory::RegisterElementInstancer(RML_POPUP_NAME, this);
    Rml::Factory::RegisterElementInstancer(RML_MENU_NAME, this);
    Rml::Factory::RegisterElementInstancer(RML_MENU_ITEM_NAME, this);
}

Rml::ElementPtr SeedRmlElementInstancer::InstanceElement(
    Rml::Element *parent, const Rml::String &tag,
    const Rml::XMLAttributes &attributes) {
    if (tag == RML_POPUP_NAME) {
        return Rml::ElementPtr(new RmlPopup(tag));
    } else if (tag == RML_MENU_NAME) {
        return Rml::ElementPtr(new RmlMenu(tag));
    } else if (tag == RML_MENU_ITEM_NAME) {
        return Rml::ElementPtr(new RmlMenuItem(tag));
    }
    return nullptr;
}

void SeedRmlElementInstancer::ReleaseElement(Rml::Element *element) {
    delete element;
}

double SeedRmlSystemInterface::GetElapsedTime() { return glfwGetTime(); }

bool SeedRmlSystemInterface::LogMessage(Rml::Log::Type type,
                                        const Rml::String &message) {
    if (type == Rml::Log::LT_ERROR || type == Rml::Log::LT_ASSERT) {
        spdlog::error("RmlUi: {}", message);
    } else if (type == Rml::Log::LT_WARNING) {
        spdlog::warn("RmlUi: {}", message);
    } else if (type == Rml::Log::LT_DEBUG) {
        spdlog::debug("RmlUi: {}", message);
    } else {
        spdlog::info("RmlUi: {}", message);
    }
    return true;
}

}  // namespace Seed