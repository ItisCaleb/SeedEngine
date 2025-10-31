#ifndef _SEED_RENDER_TARGET_H_
#define _SEED_RENDER_TARGET_H_
#include "core/rendering/api/render_resource.h"
#include "core/rendering/api/render_command.h"
#include "core/rendering/viewport.h"
#include "core/resource/texture.h"
#include "core/macro.h"
namespace Seed {
struct AttachmentSurface {
        Ref<Texture> texture;
        u8 face;
};
class RenderTarget : public RefCounted {
    protected:
        RenderResource rc;
        AttachmentSurface depth_surface;
        Viewport vp;
        bool dirty = true;
        RenderTarget() : vp(Vec2{0, 0}) {}

    public:
        RenderTarget(const Viewport &vp, bool depth_only = false);
        void bind_depth(AttachmentSurface &surface);
        void bind_depth(Ref<Texture> tex, u8 face = 0);
        AttachmentSurface &get_depth() { return this->depth_surface; };
        virtual Viewport *get_viewport() { return &vp; }
        RenderResource &get_resource() { return this->rc; }
        ~RenderTarget();
};

class MultiRenderTarget : public RenderTarget {
    private:
        AttachmentSurface color_surface[8];

    public:
        void bind_color(u32 slot, AttachmentSurface &surface);
        void bind_color(u32 slot, Ref<Texture> tex, u8 face = 0);

        AttachmentSurface &get_color(int i) {
            EXPECT_INDEX_INBOUND_THROW(i, 8);
            return color_surface[i];
        }
        MultiRenderTarget(const Viewport &vp) : RenderTarget(vp) {}
};

class WindowRenderTarget : public RenderTarget {
    private:
        WindowViewport wvp;

    public:
        WindowRenderTarget(Window *window);
        Viewport *get_viewport() override { return &wvp; }
};

}  // namespace Seed

#endif