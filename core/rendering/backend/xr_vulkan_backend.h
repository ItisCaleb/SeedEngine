#ifndef _SEED_XR_VULKAN_BACKEND_H_
#define _SEED_XR_VULKAN_BACKEND_H_
#include "core/rendering/backend/vulkan_backend.h"

namespace Seed {
class RenderBackendXRVk : public RenderBackendVK {
    protected:
        bool pick_physical_device();
        void create_instance();
        void load_xr_vulkan_funcs();
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