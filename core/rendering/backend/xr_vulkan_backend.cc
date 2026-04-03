#include "xr_vulkan_backend.h"
#include <glfw/glfw3.h>
#include <openxr/openxr.h>
#include <cstdlib>
#include <vector>
#include "core/rendering/render_common.h"
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

void RenderBackendXRVk::create_xr_reference_space(XrSession xr_session) {
    const XrPosef identity = {
        .orientation = {0, 0, 0, 1},
        .position = {0, 0, 0},
    };
    XrReferenceSpaceCreateInfo createInfo{};
    createInfo.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    createInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    createInfo.poseInReferenceSpace = identity;
    const XrResult result =
        xrCreateReferenceSpace(xr_session, &createInfo, &xr_space);
    if (XR_FAILED(result)) {
        throw std::runtime_error("Failed to create XR space!");
    }
}

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
    xrCreateVulkanInstanceKHR(xr_instance, &xrCreateInfo, &instance, &result);
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
    XREngine *xr_engine = XREngine::get_instance();
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

    XrGraphicsBindingVulkan2KHR graphicBinding{};
    graphicBinding.type = XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR;
    graphicBinding.device = device;
    graphicBinding.instance = instance;
    graphicBinding.physicalDevice = physical_device;
    graphicBinding.queueFamilyIndex = queue_family_indice;
    graphicBinding.queueIndex = 0;
    graphicBinding.next = nullptr;
    XrSession xr_session = xr_engine->bind_graphic_api(&graphicBinding);
    create_xr_reference_space(xr_session);

    create_xr_swapchain(xr_session);
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
    dummy_texture =
        this->alloc_texture(TextureType::TEXTURE_2D, 1, 1, PixelFormat::R,
                            MSAAType::SAMPLE_COUNT_1, {}, data);
}

void RenderBackendXRVk::create_xr_swapchain(XrSession xr_session) {
    XREngine *xr_engine = XREngine::get_instance();
    XrInstance xr_instance = xr_engine->get_xr_instance();
    XrSystemId xr_system_id = xr_engine->get_xr_system_id();
    XrViewConfigurationType viewType = xr_engine->get_xr_view_type();
    u32 view_count = 0;
    XrResult result = xrEnumerateViewConfigurationViews(
        xr_instance, xr_system_id, viewType, 0, &view_count, nullptr);
    std::vector<XrViewConfigurationView> config_views;
    config_views.resize(view_count);
    for (XrViewConfigurationView &view : config_views) {
        view.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
    }
    result = xrEnumerateViewConfigurationViews(
        xr_instance, xr_system_id, viewType, config_views.size(), &view_count,
        config_views.data());
    if (XR_FAILED(result)) {
        throw std::runtime_error("Failed to enumerate XR views!");
    }

    std::vector<i64> formats;

    u32 formatCount;
    xrEnumerateSwapchainFormats(xr_session, 0, &formatCount, nullptr);

    if (formatCount != 0) {
        formats.resize(formatCount);
        xrEnumerateSwapchainFormats(xr_session, formatCount, &formatCount,
                                    formats.data());
    }

    i64 target_format = formats[0];
    for (const auto &availableFormat : formats) {
        if (availableFormat == VK_FORMAT_R8G8B8A8_SRGB) {
            target_format = availableFormat;
            break;
        }
    }
    XrSwapchainCreateInfo createInfo{};
    createInfo.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    createInfo.next = nullptr;
    createInfo.createFlags = 0;
    createInfo.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                            XR_SWAPCHAIN_USAGE_TRANSFER_SRC_BIT;
    ;
    createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    createInfo.sampleCount = 1;
    createInfo.width = config_views[0].recommendedImageRectWidth;
    createInfo.height = config_views[0].recommendedImageRectHeight;
    createInfo.faceCount = 1;
    createInfo.arraySize = 1;
    createInfo.mipCount = 1;
    result = xrCreateSwapchain(xr_session, &createInfo, &xr_swap_chain.chain);

    u32 imageCount = 0;
    std::vector<XrSwapchainImageVulkan2KHR> images;
    xrEnumerateSwapchainImages(xr_swap_chain.chain, 0u, &imageCount, nullptr);

    images.resize(imageCount);
    for (XrSwapchainImageVulkan2KHR &image : images) {
        image.type = XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR;
    }

    xrEnumerateSwapchainImages(xr_swap_chain.chain, imageCount, &imageCount,
                               (XrSwapchainImageBaseHeader *)images.data());
    for (XrSwapchainImageVulkan2KHR &image : images) {
        /* since we already record format in swap chain*/
        /* we don't need to remember here */
        Handle tex = this->textures.insert(
            {.w = config_views[0].recommendedImageRectWidth,
             .h = config_views[0].recommendedImageRectHeight,
             .type = TextureType::TEXTURE_2D,
             .image = image.image});
        this->xr_swap_chain.textures.push_back(tex);
    }
    this->xr_swap_chain.format = (VkFormat)target_format;
}

void RenderBackendXRVk::create_image_views() {
    for (u32 i = 0; i < xr_swap_chain.textures.size(); i++) {
        HardwareTextureVk *tex =
            this->textures.get_or_null(xr_swap_chain.textures[i]);
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = tex->image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = xr_swap_chain.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &createInfo, nullptr, &tex->view) !=
            VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan image views!");
        }
    }
}
void RenderBackendXRVk::create_swapchain_framebuffer() {
    VkRenderPass render_pass = nullptr;
    for (u32 i = 0; i < this->xr_swap_chain.textures.size(); i++) {
        HardwareRenderPassVk rt;
        rt.is_swapchain = true;
        rt.color_attachments.push_back(HardwareColorAttachmentVk{
            .slot = 0,
            .image_format = this->xr_swap_chain.format,
            .texture_handle = this->xr_swap_chain.textures[i]});

        /* we let the swap chain to share render pass*/
        if (i == 0) {
            create_render_pass(&rt, true);
            render_pass = rt.render_pass_cache;
        } else {
            rt.render_pass_cache = render_pass;
            rt.dirty = false;
        }
        create_framebuffer(&rt);
        Handle handle = this->render_pass.insert(rt);
        this->xr_swap_chain.render_targets.push_back(handle);
    }
    current_render_target = this->xr_swap_chain.render_targets[0];
}

RenderBackendXRVk::~RenderBackendXRVk() {}
}  // namespace Seed
