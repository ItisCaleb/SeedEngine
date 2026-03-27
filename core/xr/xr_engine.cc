#include "xr_engine.h"

#include <stdexcept>
#include <vector>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <spdlog/spdlog.h>
#include "core/types.h"

namespace Seed {
#define ENGINE_NAME "The Seed"

static XRAPI_ATTR XrBool32 XRAPI_CALL
debugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity,
              XrDebugUtilsMessageTypeFlagsEXT messageTypes,
              const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    switch (messageSeverity) {
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error("OpenXR debug: {}", pCallbackData->message);
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn("OpenXR debug: {}", pCallbackData->message);
            break;
        case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        default:
            spdlog::debug("OpenXR debug: {}", pCallbackData->message);
            break;
    }
    return XR_FALSE;
}

void XREngine::load_functions() {
    XR_LOAD(xr_instance, xrCreateDebugUtilsMessengerEXT);
    XR_LOAD(xr_instance, xrDestroyDebugUtilsMessengerEXT);
}
void XREngine::create_xr_instance() {
    std::vector<XrExtensionProperties> vSupportedExt;

    // get numbers of supported extensions
    uint32_t uExtensionNum = 0;
    if (xrEnumerateInstanceExtensionProperties(nullptr, 0, &uExtensionNum,
                                               nullptr) == XR_SUCCESS) {
        if (uExtensionNum > 0) {
            // enumrate and output extension information
            vSupportedExt.resize(uExtensionNum, {XR_TYPE_EXTENSION_PROPERTIES});
            if (xrEnumerateInstanceExtensionProperties(
                    nullptr, uExtensionNum, &uExtensionNum,
                    vSupportedExt.data()) == XR_SUCCESS) {
            }
        }
    }

    std::vector<const char *> extensions = {"XR_KHR_vulkan_enable2"};
    extensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);

    XrApplicationInfo appInfo;
    appInfo.apiVersion = XR_API_VERSION_1_1;
    appInfo.applicationVersion = static_cast<u32>(XR_MAKE_VERSION(1, 0, 0));
    memcpy(appInfo.engineName, ENGINE_NAME, sizeof(ENGINE_NAME));

    XrInstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.enabledExtensionCount = extensions.size();
    instanceCreateInfo.enabledExtensionNames = extensions.data();
    instanceCreateInfo.applicationInfo = appInfo;
    const XrResult result = xrCreateInstance(&instanceCreateInfo, &xr_instance);
    if (XR_FAILED(result)) {
        throw std::runtime_error("Failed to create XR instance!");
    }
}
void XREngine::create_debug_messenger() {
    XrDebugUtilsMessengerCreateInfoEXT createInfo{
        XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    createInfo.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.messageSeverities =
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.userCallback = debugCallback;
    const XrResult result = xrCreateDebugUtilsMessengerEXT(
        xr_instance, &createInfo, &debug_messenger);
    if (XR_FAILED(result)) {
        throw std::runtime_error("Failed to create XR debug messenger!");
    }
}
void XREngine::get_system() {
    XrSystemGetInfo getInfo{};
    getInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    const XrResult result = xrGetSystem(xr_instance, &getInfo, &xr_system_id);
    if (XR_FAILED(result)) {
        throw std::runtime_error("Failed to get XR system!");
    }
}

XrSession XREngine::bind_graphic_api(void *graphicBinding) {
    XrSessionCreateInfo createInfo{};
    if (graphicBinding == nullptr) {
        throw std::runtime_error("Failed to get XR graphic binding!");
    }
    createInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    createInfo.systemId = xr_system_id;
    createInfo.next = graphicBinding;
    const XrResult result =
        xrCreateSession(xr_instance, &createInfo, &xr_session);
    if (XR_FAILED(result)) {
        throw std::runtime_error("Failed to create XR session!");
    }
    return xr_session;
}


XREngine::XREngine() {
    instance = this;
    create_xr_instance();
    load_functions();
    create_debug_messenger();
    get_system();
}
XREngine::~XREngine() {
    xrDestroyDebugUtilsMessengerEXT(debug_messenger);
    xrDestroyInstance(xr_instance);
}
}  // namespace Seed