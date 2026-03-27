#ifndef _SEED_XR_ENGINE_H_
#define _SEED_XR_ENGINE_H_

#include <openxr/openxr.h>

namespace Seed {
#define XR_FUNC(name) PFN_##name name
#define XR_LOAD(instance, name) \
    xrGetInstanceProcAddr((instance), #name, (PFN_xrVoidFunction *)name)
class XREngine {
    private:
        inline static XREngine *instance;
        XrInstance xr_instance;
        XrSystemId xr_system_id;
        XrSession xr_session;
        XrDebugUtilsMessengerEXT debug_messenger;
        void load_functions();
        void create_xr_instance();
        void create_debug_messenger();
        void get_system();

    public:
        inline static XR_FUNC(xrCreateDebugUtilsMessengerEXT);
        inline static XR_FUNC(xrDestroyDebugUtilsMessengerEXT);
        XrInstance get_xr_instance() const { return xr_instance; }
        XrSystemId get_xr_system_id() const { return xr_system_id; }
        /* return XrSession when binded */
        XrSession bind_graphic_api(void *graphic_binding);
        XrSession get_xr_session() const { return xr_session; }
        XrViewConfigurationType get_xr_view_type() {
            return XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
        }
        static XREngine *get_instance() { return instance; }
        XREngine();
        ~XREngine();
};
}  // namespace Seed

#endif