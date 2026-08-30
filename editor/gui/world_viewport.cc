#include "world_viewport.h"

#include <RmlUi/Core/Context.h>
#include <RmlUi/Core/Element.h>

#include "core/gui/gui_engine.h"
#include "core/rendering/rhi/render_engine.h"
#include "core/resource/texture.h"

namespace Seed {

namespace {

constexpr u32 kViewportWidth = 1024;
constexpr u32 kViewportHeight = 768;

}  // namespace

void WorldViewport::init() {
    if (renderer != nullptr) return;

    renderer = new WorldRenderer(kViewportWidth, kViewportHeight);
    System::gGuiEngine->add_texture("main_view",
                                    renderer->get_screen_texture());
    System::gRenderEngine->register_renderer(1, renderer);
}

void WorldViewport::reset() {
    pick_x = 0;
    pick_y = 0;
    focus_target = {};
    focus_valid = false;
    scroll_delta = 0.0f;
}

bool WorldViewport::is_hovered() const {
    Rml::Context *context = System::gGuiEngine->get_rml_context();
    Rml::Element *hovered =
        context == nullptr ? nullptr : context->GetHoverElement();
    while (hovered != nullptr) {
        if (hovered->GetId() == "viewport") return true;
        hovered = hovered->GetParentNode();
    }
    return false;
}

bool WorldViewport::event_to_pixel(Rml::Event &event, i32 &image_x,
                                   i32 &image_y) const {
    Rml::Element *element = event.GetCurrentElement();
    if (element == nullptr) return false;

    Ref<Texture> texture = renderer->get_screen_texture();
    if (texture.is_null()) return false;

    const u32 texture_width = texture->get_width();
    const u32 texture_height = texture->get_height();
    const f32 x = event.GetParameter<f32>("mouse_x", -1.0f);
    const f32 y = event.GetParameter<f32>("mouse_y", -1.0f);
    if (x < 0.0f || y < 0.0f) return false;

    const Rml::Vector2f offset = element->GetAbsoluteOffset();
    const Rml::Vector2f size = element->GetRenderBox().GetFillSize();
    if (size.x <= 0.0f || size.y <= 0.0f) return false;

    const f32 local_x = x - offset.x;
    const f32 local_y = y - offset.y;
    if (local_x < 0.0f || local_y < 0.0f || local_x >= size.x ||
        local_y >= size.y) {
        return false;
    }

    const f32 pixel_x = (local_x / size.x) * (f32)texture_width;
    const f32 pixel_y = (local_y / size.y) * (f32)texture_height;
    if (pixel_x < 0.0f || pixel_y < 0.0f || pixel_x >= texture_width ||
        pixel_y >= texture_height) {
        return false;
    }

    image_x = (i32)pixel_x;
    image_y = (i32)((f32)texture_height - 1.0f - pixel_y);
    return true;
}

bool WorldViewport::pick_at_pixel(i32 image_x, i32 image_y,
                                  PickResult &result) const {
    Ref<MappableTexture> texture = renderer->get_picking_texture();
    if (texture.is_null() || image_x < 0 || image_y < 0 ||
        image_x >= (i32)texture->get_width() ||
        image_y >= (i32)texture->get_height()) {
        return false;
    }

    i16 *pixel = (i16 *)texture->pixel((u32)image_x, (u32)image_y);
    memcpy(&result, pixel, sizeof(PickResult));
    return true;
}

void WorldViewport::set_camera_focus(const Vec3 &target) {
    focus_target = target;
    focus_valid = true;
}

void WorldViewport::clear_camera_focus() { focus_valid = false; }

bool WorldViewport::get_camera_focus(Vec3 &target) const {
    if (!focus_valid) return false;
    target = focus_target;
    return true;
}

void WorldViewport::add_scroll(f32 delta) { scroll_delta += delta; }

f32 WorldViewport::consume_scroll() {
    const f32 delta = scroll_delta;
    scroll_delta = 0.0f;
    return delta;
}

}  // namespace Seed
