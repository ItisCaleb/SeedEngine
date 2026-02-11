#include "vulkan_backend.h"
#include <GLFW/glfw3.h>
#define VOLK_IMPLEMENTATION
#include <volk.h>
#include "core/math/utils.h"

namespace Seed {

static const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

static VkDebugUtilsMessengerEXT debug_messenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            spdlog::error("Vulkan validation: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            spdlog::warn("Vulkan validation: {}", pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        default:
            spdlog::debug("Vulkan validation: {}", pCallbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

RenderBackendVK::RenderBackendVK(Window *window) {
    volkInitialize();
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
    create_surface(window);
    create_swapchain(window);
    create_image_views();
}

RenderBackendVK::~RenderBackendVK() {
    if (enable_validation) {
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
    for (auto view : swap_chain.image_views) {
        vkDestroyImageView(logical_device, view, nullptr);
    }
    vkDestroySwapchainKHR(logical_device, swap_chain.chain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(logical_device, nullptr);
    vkDestroyInstance(instance, nullptr);
}

void RenderBackendVK::create_instance() {
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

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
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
    if (enable_validation) {
        requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    create_info.enabledExtensionCount = requiredExtensions.size();
    create_info.ppEnabledExtensionNames = requiredExtensions.data();

    if (enable_validation) {
        create_info.enabledLayerCount = validationLayers.size();
        create_info.ppEnabledLayerNames = validationLayers.data();
        spdlog::debug("Enabling Vulkan validation layer");
    } else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }
}
bool RenderBackendVK::check_validation_support() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers) {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

void RenderBackendVK::create_debug_messenger() {
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
    vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debug_messenger);
}

bool RenderBackendVK::pick_physical_device() {
    u32 deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> devices(deviceCount);

    auto is_device_suitable = [&](VkPhysicalDevice device) -> bool {
        const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        // VkPhysicalDeviceProperties deviceProperties;
        // VkPhysicalDeviceFeatures deviceFeatures;
        // vkGetPhysicalDeviceProperties(device, &deviceProperties);
        // vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return true;
    };
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (auto device : devices) {
        if (is_device_suitable(device)) {
            physical_device = device;
            return true;
        }
    }
    return false;
}

void RenderBackendVK::create_logical_device() {
    if (!pick_queue_family(physical_device)) {
        throw std::runtime_error("Failed to pick a Vulkan queue family!");
    }

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queue_family_indice;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enable_validation) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &createInfo, nullptr,
                       &logical_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan logical device!");
    }

    vkGetDeviceQueue(logical_device, queue_family_indice, 0, &graphics_queue);
}

void RenderBackendVK::create_surface(Window *window) {
    if (glfwCreateWindowSurface(instance, window->get_window<GLFWwindow>(),
                                nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan window surface!");
    }
    VkBool32 present_support = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, queue_family_indice,
                                         surface, &present_support);
    if (!present_support) {
        throw std::runtime_error(
            "Presentation to the given surface not supported");
    }
}

void RenderBackendVK::create_swapchain(Window *window) {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;

    /* retrieve swap chain supports */
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface,
                                              &capabilities);
    u32 formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
        formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface,
                                             &formatCount, formats.data());
    }

    VkSurfaceFormatKHR target_format = formats[0];
    VkPresentModeKHR target_present = VK_PRESENT_MODE_FIFO_KHR;
    VkExtent2D target_extent = capabilities.currentExtent;
    for (const auto &availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            target_format = availableFormat;
            break;
        }
    }

    /* if the value of currentExtent is UINT32_MAX */
    /* then we'll decide the extent ourself */
    if (capabilities.currentExtent.width == UINT32_MAX) {
        i32 width, height;
        glfwGetFramebufferSize(window->get_window<GLFWwindow>(), &width,
                               &height);

        target_extent.width =
            clampu((u32)width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
        target_extent.height =
            clampu((u32)height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);
    }

    u32 imageCount = capabilities.minImageCount + 1;
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = target_format.format;
    createInfo.imageColorSpace = target_format.colorSpace;
    createInfo.imageExtent = target_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = target_present;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(logical_device, &createInfo, nullptr,
                             &swap_chain.chain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swap chain!");
    }
    vkGetSwapchainImagesKHR(logical_device, swap_chain.chain, &imageCount,
                            nullptr);
    swap_chain.images.resize(imageCount);
    vkGetSwapchainImagesKHR(logical_device, swap_chain.chain, &imageCount,
                            swap_chain.images.data());
    swap_chain.image_views.resize(imageCount);
    swap_chain.extent = target_extent;
    swap_chain.image_format = target_format.format;
}

void RenderBackendVK::create_image_views() {
    for (u32 i = 0; i < swap_chain.image_views.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swap_chain.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swap_chain.image_format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(logical_device, &createInfo, nullptr,
                              &swap_chain.image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan image views!");
        }
    }
}

bool RenderBackendVK::pick_queue_family(VkPhysicalDevice device) {
    u32 queue_family_cnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_cnt,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_cnt);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_cnt,
                                             queue_families.data());
    int i = 0;
    constexpr u32 flag = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT;
    for (const auto &queue_family : queue_families) {
        if (queue_family.queueFlags & flag) {
            queue_family_indice = i;
            return true;
        }
        i++;
    }
    return false;
}

void RenderBackendVK::alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                    u32 h, PixelFormat format,
                                    const SamplerProperty &property) {}

void RenderBackendVK::alloc_vertex(RenderResource *rc, u32 stride,
                                   u32 element_cnt) {}

void RenderBackendVK::alloc_indices(RenderResource *rc, IndexType type,
                                    u32 element_cnt) {}

void RenderBackendVK::alloc_shader(RenderResource *rc,
                                   const std::string &vertex_code,
                                   const std::string &fragment_code,
                                   const std::string &geometry_code,
                                   const std::string &tess_ctrl_code,
                                   const std::string &tess_eval_code) {}

void RenderBackendVK::alloc_constant(RenderResource *rc, u32 size) {}

void RenderBackendVK::alloc_pipeline(RenderResource *rc, RenderResource shader,
                                     const RenderRasterizerState &rst_state,
                                     const RenderDepthStencilState &depth_state,
                                     const RenderBlendState &blend_state) {}

void RenderBackendVK::alloc_render_target(RenderResource *rc, bool depth_only) {
}

void RenderBackendVK::alloc_buffer(RenderResource *rc, u32 size) {}

void RenderBackendVK::dealloc(RenderResource *r) {}

void RenderBackendVK::process_commands(std::deque<RenderCommand> &cmd_queue) {}

void RenderBackendVK::swap_buffer() {}

void RenderBackendVK::setup_rasterizer(const RenderRasterizerState &state) {}

void RenderBackendVK::setup_depth_stencil(
    const RenderDepthStencilState &state) {}

void RenderBackendVK::setup_blend(const RenderBlendState &state) {}

void RenderBackendVK::handle_alloc(AllocCommand &cmd) {}

void RenderBackendVK::handle_dealloc(AllocCommand &cmd) {}

void RenderBackendVK::handle_update(RenderCommand &cmd) {}

void RenderBackendVK::handle_state(RenderCommand &cmd) {}

void RenderBackendVK::handle_render(RenderCommand &cmd) {}

void RenderBackendVK::use_vertex_desc(VertexLayout *desc) {}

void RenderBackendVK::bind_buffer(RenderResource &rc) {}

void RenderBackendVK::use_shader(RenderResource &rc) {}

void RenderBackendVK::use_texture(u32 unit, RenderResource &rc) {}

}  // namespace Seed