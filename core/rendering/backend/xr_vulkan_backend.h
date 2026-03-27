#ifndef _SEED_XR_VULKAN_BACKEND_H_
#define _SEED_XR_VULKAN_BACKEND_H_
#include "core/rendering/backend/vulkan_backend.h"
#include <openxr/openxr.h>

namespace Seed {
class RenderBackendXRVk : public RenderBackendVK {
    protected:
        struct XRSwapChain {
                XrSwapchain chain;
                VkFormat format;
                std::vector<Handle> textures;
                std::vector<Handle> render_targets;
                std::vector<VkSemaphore> semaphore;
                u32 next_index = 0;
        } xr_swap_chain;
        bool pick_physical_device();
        void create_instance();
        void load_xr_vulkan_funcs();
        void create_xr_reference_space(XrSession xr_session);
        void create_xr_swapchain(XrSession xr_session);
        void create_image_views();
        void create_swapchain_framebuffer();
        XrSpace xr_space;

    public:
        RenderBackendXRVk(Window *window);
        ~RenderBackendXRVk();
        inline RenderBackendType get_type() override {
            return RenderBackendType::XR_VULKAN;
        }
        void swap_buffer() override;
};

}  // namespace Seed

#endif