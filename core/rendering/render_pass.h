#ifndef _SEED_RENDER_PASS_H_
#define _SEED_RENDER_PASS_H_
#include <string>
#include <vector>
#include "core/rendering/viewport.h"
#include "core/resource/texture.h"
#include "core/rendering/rhi/render_command.h"
#include "core/window.h"

namespace Seed {

struct Attachment {
        Ref<Texture> texture;
        u32 texture_layer;
        u8 slot = -1;
        bool clear;
};

template <typename T>
class RenderPass {
    protected:
        RenderPassHandle handle = NULL_HANDLE;
        Viewport viewport;
        StateClearFlag clear_flag = CLEAR_COLOR | CLEAR_DEPTH | CLEAR_STENCIL;
        std::string name;
        /* if there is no attachment, we use swap chain as attachment */
        std::vector<Attachment> color_attachments;
        Attachment depth_stencil_attachment;
        virtual void execute(RenderCommandDispatcher &dp, Viewport &viewport,
                             T &fd) = 0;

    public:
        RenderPass() : viewport(0, 0) { handle = RHI::alloc_renderpass(); }
        ~RenderPass() { RHI::dealloc(handle); }
        void set_viewport(const Viewport &viewport) {
            this->viewport = viewport;
        }
        void set_name(const std::string &name) { this->name = name; }
        void set_clear_flag(StateClearFlag clear_flag) {
            this->clear_flag = clear_flag;
        }
        void bind_color_attachment(Ref<Texture> texture, u32 layer, u8 slot) {
            for (Attachment &attachment : color_attachments) {
                if (attachment.slot == slot) {
                    attachment.texture = texture;
                    attachment.texture_layer = layer;
                    RHI::bind_color_attachment(handle, slot,
                                               texture->get_handle(), layer);
                    return;
                }
            }
            color_attachments.push_back(Attachment{
                .texture = texture, .texture_layer = layer, .slot = slot});
            RHI::bind_color_attachment(handle, slot, texture->get_handle(),
                                       layer);
        }
        void bind_depth_attachment(Ref<Texture> texture, u32 layer) {
            this->depth_stencil_attachment.texture = texture;
            this->depth_stencil_attachment.texture_layer = layer;
            RHI::bind_depth_attachment(handle, texture->get_handle(), layer);
        }
        void set_renderpass(RenderPass<T> &render_pass) {
            this->viewport = render_pass.viewport;
            this->handle = render_pass.handle;
        }
        void draw(RenderCommandDispatcher &dp, T &frame_data) {
            dp.begin_scope(this->name);
            RenderStateDataBuilder state;
            state.bind_render_pass(this->handle);
            state.set_scissor(this->viewport.get_actual_dimension());
            state.set_viewport(&this->viewport);
            state.clear(this->clear_flag);
            dp.set_states(state);
            this->execute(dp, this->viewport, frame_data);
            dp.end_scope();
        }
};

template <typename T>
class WindowRenderPass : public RenderPass<T> {
    private:
        Window *window;

    public:
        WindowRenderPass() { this->handle = NULL_HANDLE; }
        ~WindowRenderPass() {}
        void bind_color_attachment(Ref<Texture> texture, u32 layer,
                                   u8 slot) = delete;
        void bind_depth_attachment(Ref<Texture> texture, u32 layer) = delete;
        void set_window(Window *window) { this->window = window; }
        void draw(RenderCommandDispatcher &dp, T &frame_data) {
            if (!window) {
                return;
            }
            dp.begin_scope(this->name);
            RenderStateDataBuilder state;
            Viewport vp(window->get_width(), window->get_height());
            state.bind_render_pass(NULL_HANDLE);
            state.set_scissor(vp.get_actual_dimension());
            state.set_viewport(&vp);
            state.clear(this->clear_flag);
            dp.set_states(state);
            this->execute(dp, vp, frame_data);
            dp.end_scope();
        }
};
}  // namespace Seed

#endif
