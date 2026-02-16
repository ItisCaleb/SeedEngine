#include "render_target.h"
#include "core/engine.h"

namespace Seed {
RenderTarget::RenderTarget(const Viewport &vp, bool depth_only) : vp(vp) {
    handle = RHI::alloc_render_target(depth_only);
}

void RenderTarget::bind_depth(AttachmentSurface &surface) {
    this->depth_surface = surface;
    RHI::bind_depth_attachment(handle, surface.texture->get_handle(),
                               surface.face);
}

void RenderTarget::bind_depth(Ref<Texture> tex, u8 face) {
    this->depth_surface = AttachmentSurface{tex, face};
    RHI::bind_depth_attachment(handle, tex->get_handle(), face);
}

RenderTarget::~RenderTarget() { RHI::dealloc(handle); }

void MultiRenderTarget::bind_color(u8 slot, AttachmentSurface &surface) {
    EXPECT_INDEX_INBOUND_THROW(slot, 8);
    this->color_surface[slot] = surface;
    RHI::bind_color_attachment(handle, slot, surface.texture->get_handle(),
                               surface.face);
}

void MultiRenderTarget::bind_color(u8 slot, Ref<Texture> tex, u32 face) {
    EXPECT_INDEX_INBOUND_THROW(slot, 8);
    this->color_surface[slot] = AttachmentSurface{tex, face};
    RHI::bind_color_attachment(handle, slot, tex->get_handle(), face);
}

WindowRenderTarget::WindowRenderTarget(Window *window) : wvp(window) { this->handle = NULL_HANDLE;}
}  // namespace Seed