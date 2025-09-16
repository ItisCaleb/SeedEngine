#include "render_target.h"
#include "core/engine.h"

namespace Seed {
RenderTarget::RenderTarget()
    : viewport(SeedEngine::get_instance()->get_window()) {
    this->rc.alloc_render_target();
}

void RenderTarget::bind_depth(AttachmentSurface &surface) {
    this->depth_surface = surface;
    RenderCommandDispatcher dp;
    dp.update_depth_attachment(this->rc, surface.texture->get_render_resource(),
                               surface.face);
}

void RenderTarget::bind_depth(Ref<Texture> tex, u8 face){
    this->depth_surface = AttachmentSurface{tex, face};
    RenderCommandDispatcher dp;
    dp.update_depth_attachment(this->rc, tex->get_render_resource(),
                               face);
}

RenderTarget::~RenderTarget() { this->rc.dealloc(); }

void MultiRenderTarget::bind_color(u32 slot, AttachmentSurface &surface) {
    EXPECT_INDEX_INBOUND_THROW(slot, 8);
    this->color_surface[slot] = surface;
    RenderCommandDispatcher dp;

    dp.update_color_attachment(
        this->rc, 0, surface.texture->get_render_resource(), surface.face);
}

void MultiRenderTarget::bind_color(u32 slot, Ref<Texture> tex, u8 face){
    EXPECT_INDEX_INBOUND_THROW(slot, 8);
    this->color_surface[slot] = AttachmentSurface{tex, face};
    RenderCommandDispatcher dp;
    dp.update_color_attachment(this->rc, 0, tex->get_render_resource(),
                               face);
}

WindowRenderTarget::WindowRenderTarget() {
    /* We don't allocate a framebuffer for window */
    this->rc.dealloc();
}
}  // namespace Seed