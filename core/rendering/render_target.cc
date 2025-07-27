#include "render_target.h"
#include "core/engine.h"

namespace Seed {
RenderTarget::RenderTarget()
    : viewport(SeedEngine::get_instance()->get_window()) {
    this->rc.alloc_render_target();
}

void RenderTarget::bind_depth(AttachmentSurface &surface) {
    RenderCommandDispatcher dp(0);
    dp.update_depth_attachment(this->rc, surface.texture->get_render_resource(),
                               surface.face);
}

RenderTarget::~RenderTarget() { this->rc.dealloc(); }

void MultiRenderTarget::bind_color(u32 slot, AttachmentSurface &surface) {
    RenderCommandDispatcher dp(0);
    dp.update_color_attachment(
        this->rc, 0, surface.texture->get_render_resource(), surface.face);
}

WindowRenderTarget::WindowRenderTarget() {
    /* We don't allocate a framebuffer for window */
    this->rc.dealloc();
}
}  // namespace Seed