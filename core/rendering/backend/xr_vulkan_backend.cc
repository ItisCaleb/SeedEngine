#include "xr_vulkan_backend.h"
#include <glfw/glfw3.h>
#define VOLK_IMPLEMENTATION
#include <volk.h>
#define VMA_IMPLEMENTATION
#ifdef __APPLE__
#include <vk_mem_alloc.h>
#else
#include <vma/vk_mem_alloc.h>
#endif
#include "core/xr/xr_engine.h"
#define XR_USE_GRAPHICS_API_VULKAN

#include <openxr/openxr_platform.h>

namespace Seed {

static const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static XR_FUNC(xrCreateVulkanInstanceKHR);
static XR_FUNC(xrCreateVulkanDeviceKHR);
static XR_FUNC(xrGetVulkanGraphicsDevice2KHR);
static XR_FUNC(xrGetVulkanGraphicsRequirements2KHR);

bool RenderBackendXRVk::pick_physical_device() { return false; }

void RenderBackendXRVk::create_instance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion =
        VK_MAKE_API_VERSION(0, 1, 0, 0);  // Using the macro
    appInfo.pEngineName = "The Seed";
    appInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char *> requiredExtensions;

    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.emplace_back(glfwExtensions[i]);
    }

#ifdef __APPLE__
    /* for MacOS compatibility */
    requiredExtensions.emplace_back(
        VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    if (enable_validation) {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (enable_validation) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
        spdlog::debug("Enabling Vulkan validation layer");
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
    XrInstance xr_instance = XREngine::get_instance()->get_xr_instance();
    XrSystemId xr_system_id = XREngine::get_instance()->get_xr_system_id();

    XrVulkanInstanceCreateInfoKHR xrCreateInfo{};
    xrCreateInfo.type = XR_TYPE_VULKAN_INSTANCE_CREATE_INFO_KHR;
    xrCreateInfo.vulkanCreateInfo = &createInfo;
    xrCreateInfo.systemId = xr_system_id;
    xrCreateInfo.pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
    VkResult result;
    xrCreateVulkanInstanceKHR(xr_instance, &xrCreateInfo, &instance,
                                        &result);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create XR Vulkan Instance!");
    }
}

void RenderBackendXRVk::load_xr_vulkan_funcs() {
    XrInstance xr_instance = XREngine::get_instance()->get_xr_instance();
    XR_LOAD(xr_instance, xrCreateVulkanInstanceKHR);
    XR_LOAD(xr_instance, xrCreateVulkanDeviceKHR);
    XR_LOAD(xr_instance, xrGetVulkanGraphicsDevice2KHR);
    XR_LOAD(xr_instance, xrGetVulkanGraphicsRequirements2KHR);
}

RenderBackendXRVk::RenderBackendXRVk(Window *window) {
    current_window = window;
    volkInitialize();
    load_xr_vulkan_funcs();
    create_instance();
    volkLoadInstance(instance);
    if (enable_validation) {
        create_debug_messenger();
    }
    if (!pick_physical_device()) {
        spdlog::error("Can't pick a suitable GPU.");
        throw std::runtime_error("");
    }
    create_logical_device();
    volkLoadDevice(device);
    create_surface(window);
    create_swapchain(window);
    create_image_views();
    create_swapchain_framebuffer();
    create_command_pool();
    create_command_buffer();
    create_descriptor_pool();
    create_sync_objects();
    VmaAllocatorCreateInfo createInfo{};
    VmaVulkanFunctions funcInfos{};
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    createInfo.device = device;
    createInfo.instance = instance;
    createInfo.physicalDevice = physical_device;
    vmaImportVulkanFunctionsFromVolk(&createInfo, &funcInfos);
    createInfo.pVulkanFunctions = &funcInfos;
    vmaCreateAllocator(&createInfo, &buffer_allocator);
    dummy_constant =
        this->alloc_constant(1, nullptr, UpdateFrequence::PERFRAME);
    dummy_ssbo =
        this->alloc_storage_buffer(1, nullptr, UpdateFrequence::PERFRAME);
    u8 *data = (u8 *)malloc(1);
    data[0] = 0;
    dummy_texture = this->alloc_texture(TextureType::TEXTURE_2D, 1, 1,
                                        PixelFormat::R, {}, data);
}
RenderBackendXRVk::~RenderBackendXRVk() {}
}  // namespace Seed
