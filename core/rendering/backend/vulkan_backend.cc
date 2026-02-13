#include "vulkan_backend.h"
#include <GLFW/glfw3.h>
#include "core/rendering/backend/vulkan_helper.h"
#define VOLK_IMPLEMENTATION
#include <volk.h>
#include <vma/vk_mem_alloc.h>
#include "core/math/utils.h"
#include "core/macro.h"
#include "core/misc/hash.h"
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>
#include <utility>

namespace Seed {

static const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

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
    create_swapchain_framebuffer();
    create_command_pool();
    create_command_buffer();
    VmaAllocatorCreateInfo createInfo{};
    VmaVulkanFunctions funcInfos{};
    createInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    createInfo.device = device;
    createInfo.instance = instance;
    createInfo.physicalDevice = physical_device;
    vmaImportVulkanFunctionsFromVolk(&createInfo, &funcInfos);
    createInfo.pVulkanFunctions = &funcInfos;
    vmaCreateAllocator(&createInfo, &buffer_allocator);
}

RenderBackendVK::~RenderBackendVK() {
    if (enable_validation) {
        vkDestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
    vkDestroyCommandPool(device, command_pool, nullptr);
    for (Handle handle : swap_chain.textures) {
        HardwareTextureVk *tex = textures.get_or_null(handle);
        vkDestroyImageView(device, tex->view, nullptr);
        vkDestroyImage(device, tex->image, nullptr);
    }
    vkDestroySwapchainKHR(device, swap_chain.chain, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
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

        // VkPhysicalDeviceFeatures deviceFeatures;
        // vkGetPhysicalDeviceProperties(device, &deviceProperties);
        // vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        return true;
    };
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    for (auto device : devices) {
        if (is_device_suitable(device)) {
            physical_device = device;
            vkGetPhysicalDeviceProperties(device, &device_properties);
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
    deviceFeatures.tessellationShader = true;
    deviceFeatures.samplerAnisotropy = true;

    constexpr VkPhysicalDeviceDynamicRenderingFeaturesKHR
        dynamic_rendering_feature{
            .sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
            .dynamicRendering = VK_TRUE,
        };

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pNext = &dynamic_rendering_feature;
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enable_validation) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &createInfo, nullptr, &device) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan logical device!");
    }

    vkGetDeviceQueue(device, queue_family_indice, 0, &graphics_queue);
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
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swap_chain.chain) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan swap chain!");
    }

    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(device, swap_chain.chain, &imageCount, nullptr);

    images.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swap_chain.chain, &imageCount,
                            images.data());
    for (VkImage image : images) {
        /* since we already record format in swap chain*/
        /* we don't need to remember here */
        Handle tex = this->textures.insert({.w = target_extent.width,
                                            .h = target_extent.height,
                                            .type = TextureType::TEXTURE_2D,
                                            .image = image});
        this->swap_chain.textures.push_back(tex);
    }
    this->swap_chain.format = target_format.format;
}

void RenderBackendVK::create_image_views() {
    for (u32 i = 0; i < swap_chain.textures.size(); i++) {
        HardwareTextureVk *tex =
            this->textures.get_or_null(swap_chain.textures[i]);
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = tex->image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swap_chain.format;
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

void RenderBackendVK::create_swapchain_framebuffer() {
    VkRenderPass render_pass = nullptr;
    for (u32 i = 0; i < this->swap_chain.textures.size(); i++) {
        HardwareRenderTargetVk rt;
        rt.attachments.push_back(HardwareAttachmentVk{
            .slot = 0,
            .image_format = this->swap_chain.format,
            .texture_handle = this->swap_chain.textures[i]});

        /* we let the swap chain to share render pass*/
        if (i == 0) {
            create_render_pass(&rt, true);
            render_pass = rt.render_pass_cache;
        } else {
            rt.render_pass_cache = render_pass;
        }
        create_framebuffer(&rt);
        Handle handle = this->render_targets.insert(rt);
        this->swap_chain.render_targets.push_back(handle);
    }
    current_render_target =
        this->render_targets.get_or_null(this->swap_chain.render_targets[0]);
}

void RenderBackendVK::create_command_pool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queue_family_indice;
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &command_pool) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan command pool!");
    }
}

void RenderBackendVK::create_command_buffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = command_pool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &command_buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void RenderBackendVK::create_staging_buffer(VkBuffer *buffer,
                                            VmaAllocation *allocation,
                                            u64 size) {
    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    stagingBufferInfo.size = size;

    VmaAllocationCreateInfo stagingAllocInfo = {};
    stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    stagingAllocInfo.flags =
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vmaCreateBuffer(buffer_allocator, &stagingBufferInfo, &stagingAllocInfo,
                    buffer, allocation, nullptr);
}

void RenderBackendVK::create_host_visible_buffer(VkBuffer *buffer,
                                                 VmaAllocation *allocation,
                                                 VkBufferUsageFlagBits usage,
                                                 u64 size, const void *data) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = size;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;

    vmaCreateBuffer(buffer_allocator, &bufferInfo, &allocInfo, buffer,
                    allocation, nullptr);

    if (data) {
        vmaCopyMemoryToAllocation(buffer_allocator, data, *allocation, 0,
                                  bufferInfo.size);
    }
}

void RenderBackendVK::create_gpu_only_buffer(VkBuffer *buffer,
                                             VmaAllocation *allocation,
                                             VkBufferUsageFlagBits usage,
                                             u64 size, const void *data) {
    if (!data) {
        spdlog::warn("Creating a buffer without data provided.");
    }

    /* create target buffer */
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = size;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    vmaCreateBuffer(buffer_allocator, &bufferInfo, &allocInfo, buffer,
                    allocation, nullptr);

    /* copy to dedicated memory later */
    if (data) {
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
        vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                                  bufferInfo.size);
        this->buffer_copy_queue.push(BufferCopy{.staging_buffer = stagingBuffer,
                                                .target_buffer = *buffer,
                                                .size = size});
        this->destroy_queue.push(DestroyResource{
            .type = RenderResourceType::VERTEX,
            .buffer = {.buffer = stagingBuffer, .memory = stagingAllocation}});
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

void RenderBackendVK::reallocate_buffer(HardwareBufferVk *buffer,
                                        VkBufferUsageFlagBits usage, u64 size) {
    VkBuffer new_buffer;
    VmaAllocation allocation;

    /* create new buffer */
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = usage;

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = size;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
    vmaCreateBuffer(buffer_allocator, &bufferInfo, &allocInfo, &new_buffer,
                    &allocation, nullptr);

    /* copy data */
    void *old_mem;
    void *new_mem;
    vmaMapMemory(buffer_allocator, buffer->memory, &old_mem);
    vmaMapMemory(buffer_allocator, allocation, &new_mem);
    memcpy(old_mem, new_mem, buffer->size);
    vmaUnmapMemory(buffer_allocator, buffer->memory);
    vmaUnmapMemory(buffer_allocator, allocation);

    this->destroy_queue.push(DestroyResource{
        .type = RenderResourceType::VERTEX,
        .buffer = {.buffer = buffer->buffer, .memory = buffer->memory}});

    /* rebind */
    buffer->size = size;
    buffer->buffer = new_buffer;
    buffer->memory = allocation;
}

void RenderBackendVK::alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                    u32 h, PixelFormat format,
                                    const SamplerProperty &property,
                                    const void *data) {
    VkImage image;
    VkImageView image_view;
    VkSampler sampler;
    VkImageCreateInfo imageInfo{};

    bool is_depth_stencil = false;
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    switch (format) {
        case PixelFormat::D24:
        case PixelFormat::D24S8:
        case PixelFormat::D32:
        case PixelFormat::D32S8:
            is_depth_stencil = true;
            break;
        default:
            break;
    }
    if (is_depth_stencil) {
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    } else {
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT |
                          VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.format = VulkanHelper::texture_format(format);
    imageInfo.imageType = VulkanHelper::texture_type(type);
    imageInfo.extent = VkExtent3D{.width = w, .height = h, .depth = 1};
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.mipLevels = 1;
    if (type == TextureType::TEXTURE_CUBEMAP) {
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.arrayLayers = 6;
    } else {
        imageInfo.arrayLayers = 1;
    }
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocation allocation;
    vmaCreateImage(buffer_allocator, &imageInfo, &allocInfo, &image,
                   &allocation, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = imageInfo.format;
    viewInfo.subresourceRange.aspectMask = is_depth_stencil
                                               ? VK_IMAGE_ASPECT_DEPTH_BIT
                                               : VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.layerCount = imageInfo.arrayLayers;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.levelCount = imageInfo.mipLevels;
    viewInfo.viewType = VulkanHelper::texture_view_type(type);
    viewInfo.image = image;
    vkCreateImageView(device, &viewInfo, nullptr, &image_view);

    VkSamplerCreateInfo samplerInfo = VulkanHelper::sampler_info(
        property, device_properties.limits.maxSamplerAnisotropy);

    if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    VkDescriptorImageInfo imageDescriptor{};
    imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageDescriptor.imageView = image_view;
    imageDescriptor.sampler = sampler;

    this->textures.insert({.w = w,
                           .h = h,
                           .type = type,
                           .format = format,
                           .image = image,
                           .view = image_view,
                           .sampler = sampler,
                           .memory = allocation});

    /* upload using staging buffer */
    /* we don't updload cubemap here */
    if (data && type != TextureType::TEXTURE_CUBEMAP) {
        size_t size = w * h * get_pixel_format_size(format);
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
        vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                                  size);
        this->image_copy_queue.push(ImageCopy{.staging_buffer = stagingBuffer,
                                              .target_image = image,
                                              .w = w,
                                              .h = h});

        this->destroy_queue.push(DestroyResource{
            .type = RenderResourceType::VERTEX,
            .buffer = {.buffer = stagingBuffer, .memory = stagingAllocation}});
    }
}

void RenderBackendVK::alloc_vertex(RenderResource *rc, u32 stride,
                                   u32 element_cnt, UpdateFrequence frequence,
                                   const void *data) {
    VkBuffer vertex;
    VmaAllocation allocation;

    if (frequence == UpdateFrequence::IMMUTABLE) {
        create_gpu_only_buffer(&vertex, &allocation,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               stride * element_cnt, data);
    } else {
        create_host_visible_buffer(&vertex, &allocation,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   stride * element_cnt, data);
    }

    this->vertices.insert({.buffer = vertex,
                           .memory = allocation,
                           .frequence = frequence,
                           .size = stride * element_cnt});
}

void RenderBackendVK::alloc_indices(RenderResource *rc, IndexType type,
                                    u32 element_cnt, UpdateFrequence frequence,
                                    const void *data) {
    VkBuffer indice;
    VmaAllocation allocation;
    u64 size = get_index_size(type) * element_cnt;
    if (frequence == UpdateFrequence::IMMUTABLE) {
        create_gpu_only_buffer(&indice, &allocation,
                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size, data);
    } else {
        create_host_visible_buffer(
            &indice, &allocation, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size, data);
    }

    HardwareIndexVk index;
    index.type = type;
    index.buffer = indice;
    index.size = size;
    index.frequence = frequence;
    index.memory = allocation;
    this->indices.insert(index);
}

VkShaderModule RenderBackendVK::create_shader_module(
    const std::string &shader) {
    VkShaderModule module = nullptr;
    if (shader.empty()) return module;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shader.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(shader.data());
    vkCreateShaderModule(device, &createInfo, nullptr, &module);
    return module;
}

void RenderBackendVK::alloc_shader(RenderResource *rc,
                                   const std::string &vertex_code,
                                   const std::string &fragment_code,
                                   const std::string &geometry_code,
                                   const std::string &tess_ctrl_code,
                                   const std::string &tess_eval_code) {
    rc->handle =
        shaders.insert(HardwareShaderVk{.vertex_src = vertex_code,
                                        .geo_src = geometry_code,
                                        .tess_ctrl_src = tess_ctrl_code,
                                        .tess_eval_src = tess_eval_code,
                                        .fragment_src = fragment_code,
                                        .set_layout = nullptr,
                                        .layout = nullptr});
}

void RenderBackendVK::setup_shader_layout(RenderResource *rc,
                                          const ShaderLayout &shader_layout) {
    HardwareShaderVk *shader = this->shaders.get_or_null(rc->handle);
    EXPECT_NOT_NULL_RET(shader);
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    std::vector<VkDescriptorSetLayoutBinding> layouts;
    std::vector<VkPushConstantRange> ranges;

    /* only 1 set for now*/
    VkDescriptorSetLayout set_layout;

    /* bindings */
    for (const ShaderBinding &binding : shader_layout.bindings) {
        VkDescriptorSetLayoutBinding _binding{};
        _binding.binding = binding.binding_point;
        _binding.descriptorCount = binding.count;
        _binding.descriptorType = VulkanHelper::descriptor_type(binding.type);
        _binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        layouts.push_back(_binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layouts.size();
    layoutInfo.pBindings = layouts.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                    &set_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }

    for (const PushConstantRange &range : shader_layout.push_constants) {
        VkPushConstantRange _range;
        _range.offset = range.offset;
        _range.size = range.size;
        _range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        ranges.push_back(_range);
    }

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &set_layout;
    pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
    pipelineLayoutInfo.pPushConstantRanges = ranges.data();
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan pipeline layout!");
    }
    shader->set_layout = set_layout;
    shader->layout = pipelineLayout;
}

void RenderBackendVK::alloc_constant(RenderResource *rc, u32 size,
                                     const void *data) {
    VkBuffer constant;
    VmaAllocation allocation;
    create_host_visible_buffer(&constant, &allocation,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, data);
    this->constants.insert({.buffer = constant,
                            .memory = allocation,
                            .frequence = UpdateFrequence::PERFRAME,
                            .size = size});
}

void RenderBackendVK::alloc_buffer(RenderResource *rc, u32 size,
                                   const void *data) {
    VkBuffer buffer;
    VmaAllocation allocation;

    create_host_visible_buffer(&buffer, &allocation,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size, data);
    this->ssbos.insert({.buffer = buffer,
                        .memory = allocation,
                        .frequence = UpdateFrequence::PERFRAME,
                        .size = size});
}

void RenderBackendVK::alloc_pipeline(RenderResource *rc, RenderResource shader,
                                     const RenderRasterizerState &rst_state,
                                     const RenderDepthStencilState &depth_state,
                                     const RenderBlendState &blend_state) {
    HardwareShaderVk *_shader = shaders.get_or_null(shader.handle);
    if (!_shader) {
        throw std::runtime_error(
            "failed to create Vulkan pipeline layout! Shader is null.");
    }

    HardwarePipelineVk pipeline;
    pipeline.rst_state = rst_state;
    pipeline.depth_state = depth_state;
    pipeline.blend_attachment = blend_state;
    this->pipelines.insert(pipeline);
}

void RenderBackendVK::create_render_pass(HardwareRenderTargetVk *render_target,
                                         bool is_swapchain) {
    VkSubpassDescription subpass{};
    std::vector<VkAttachmentDescription> allAttachments;
    std::vector<VkAttachmentDescription> colorAttachments;
    std::vector<VkAttachmentReference> colorRefs;
    VkAttachmentDescription depthAttachment{};
    VkAttachmentReference depthRef{};
    u32 i = 0;

    if (!render_target->dirty) {
        return;
    }
    render_target->dirty = false;

    if (render_target->render_pass_cache != nullptr) {
        vkDestroyRenderPass(device, render_target->render_pass_cache, nullptr);
    }

    for (auto attachment : render_target->attachments) {
        if (attachment.is_depth) {
            depthAttachment.format = attachment.image_format;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout =
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            allAttachments.push_back(depthAttachment);

            depthRef.attachment = i;
            depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            subpass.pDepthStencilAttachment = &depthRef;
        } else {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = attachment.image_format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout =
                is_swapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                             : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorAttachments.push_back(colorAttachment);
            allAttachments.push_back(colorAttachment);

            VkAttachmentReference colorRef;
            colorRef.attachment = i;
            colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorRefs.push_back(colorRef);
        }
        i++;
    }
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorRefs.size();
    subpass.pColorAttachments = colorRefs.data();

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = allAttachments.size();
    renderPassInfo.pAttachments = allAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr,
                           &render_target->render_pass_cache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan render pass!");
    }
}

void RenderBackendVK::create_framebuffer(
    HardwareRenderTargetVk *render_target) {
    if (!render_target->texture_changed) {
        return;
    }
    render_target->texture_changed = false;

    if (render_target->framebuffer_cache != nullptr) {
        vkDestroyFramebuffer(device, render_target->framebuffer_cache, nullptr);
    }

    VkFramebufferCreateInfo framebufferInfo{};
    std::vector<VkImageView> attachments;
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = render_target->render_pass_cache;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.layers = 1;
    u32 width = 0;
    u32 height = 0;
    for (auto &attachment : render_target->attachments) {
        HardwareTextureVk *texture =
            this->textures.get_or_null(attachment.texture_handle);
        attachments.push_back(texture->view);
        width = std::max(texture->w, framebufferInfo.width);
        height = std::max(texture->h, framebufferInfo.height);
    }
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.pAttachments = attachments.data();
    render_target->w = width;
    render_target->h = height;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &render_target->framebuffer_cache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan framebuffer!");
    }
}

VkPipeline RenderBackendVK::get_vk_pipeline(
    HardwarePipelineVk *pipeline, HardwareRenderTargetVk *render_target,
    std::vector<VertexLayout *> &layouts) {
    Hash _hash;
    _hash.update(&pipeline->rst_state.cull_mode);
    _hash.update(&pipeline->rst_state.poly_mode);
    _hash.update(&pipeline->rst_state.patch_control_points);

    _hash.update(&pipeline->depth_state.depth_on);
    _hash.update(&pipeline->depth_state.depth_compare_op);
    _hash.update(&pipeline->depth_state.stencil_on);
    _hash.update(&pipeline->depth_state.stencil_compare_op);

    _hash.update(&pipeline->blend_attachment.blend_on);
    _hash.update(&pipeline->blend_attachment.func.src_rgb);
    _hash.update(&pipeline->blend_attachment.func.dst_rgb);
    _hash.update(&pipeline->blend_attachment.func.src_alpha);
    _hash.update(&pipeline->blend_attachment.func.dst_alpha);

    /* since we usually don't create multiple shader/render target/layout with
     * same config */
    /* we just hash its handle and address here*/
    _hash.update(&pipeline->shader.handle);
    _hash.update(&render_target->render_pass_cache);
    for (auto layout : layouts) {
        _hash.update(&layout);
    }
    u64 hash = _hash.digest();
    VkPipeline vk_pipeline;
    auto iter = pipeline_cache.find(hash);
    if (iter == pipeline_cache.end()) {
        vk_pipeline = create_vk_pipeline(pipeline, render_target, layouts);
        pipeline_cache.emplace(hash, vk_pipeline);
    } else {
        vk_pipeline = iter->second;
    }
    return vk_pipeline;
}

VkPipeline RenderBackendVK::create_vk_pipeline(
    HardwarePipelineVk *pipeline, HardwareRenderTargetVk *render_target,
    std::vector<VertexLayout *> &layouts) {
    VkPipeline graphicsPipeline;
    HardwareShaderVk *shader =
        this->shaders.get_or_null(pipeline->shader.handle);
    if (!shader) {
        throw std::runtime_error(
            "failed to create graphics pipeline! No shader provided");
    }
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY};

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount =
        static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    /* we use dynamic, so only set number here */
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterState =
        VulkanHelper::rasterizer(pipeline->rst_state);
    VkPipelineDepthStencilStateCreateInfo depthState =
        VulkanHelper::depth_stencil(pipeline->depth_state);

    VkPipelineColorBlendAttachmentState blendAttachment =
        VulkanHelper::blend_attachment(pipeline->blend_attachment);
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &blendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    /* create shader stages */
    std::vector<VkPipelineShaderStageCreateInfo> stageInfos;

    VkShaderModule vert = create_shader_module(shader->vertex_src);
    VkShaderModule frag = create_shader_module(shader->fragment_src);
    VkShaderModule geom = create_shader_module(shader->geo_src);
    VkShaderModule tesc = create_shader_module(shader->tess_ctrl_src);
    VkShaderModule tese = create_shader_module(shader->tess_eval_src);
    auto create_stage_info = [&](const char *entry, VkShaderModule module,
                                 VkShaderStageFlagBits stage) {
        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = stage;
        stageInfo.module = module;
        stageInfo.pName = entry;
        return stageInfo;
    };
    stageInfos.push_back(
        create_stage_info("vert", vert, VK_SHADER_STAGE_VERTEX_BIT));
    if (geom) {
        stageInfos.push_back(
            create_stage_info("geom", geom, VK_SHADER_STAGE_GEOMETRY_BIT));
    }
    if (tesc) {
        stageInfos.push_back(create_stage_info(
            "tesc", tesc, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT));
    }
    if (tese) {
        stageInfos.push_back(create_stage_info(
            "tese", tese, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT));
    }
    if (frag) {
        stageInfos.push_back(
            create_stage_info("frag", frag, VK_SHADER_STAGE_FRAGMENT_BIT));
    }
    std::vector<VkVertexInputAttributeDescription> attr_desc;
    std::vector<VkVertexInputBindingDescription> binding_desc;

    u32 i = 0;
    for (auto layout : layouts) {
        VulkanHelper::vertex_layout(layout, i, attr_desc, binding_desc);
        i++;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.pVertexAttributeDescriptions = attr_desc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = attr_desc.size();
    vertexInputInfo.pVertexBindingDescriptions = binding_desc.data();
    vertexInputInfo.vertexAttributeDescriptionCount = binding_desc.size();

    /* create render pass */
    /* since user will bind render target's attachment */
    /* at update, we need to create render pass here. */
    create_render_pass(render_target, false);

    /* create pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = stageInfos.size();
    pipelineInfo.pStages = stageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterState;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthState;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = shader->layout;
    pipelineInfo.renderPass = render_target->render_pass_cache;
    pipelineInfo.subpass = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                  nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    return graphicsPipeline;
}

void RenderBackendVK::alloc_render_target(RenderResource *rc, bool depth_only) {
    rc->handle = this->render_targets.insert({.depth_only = depth_only});
}

void RenderBackendVK::dealloc(RenderResource *rc) {
    switch (rc->type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferVk *vertices = this->vertices.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(vertices);
            this->destroy_queue.push(
                DestroyResource{.type = RenderResourceType::VERTEX,
                                .buffer = {.buffer = vertices->buffer,
                                           .memory = vertices->memory}});
            vmaDestroyBuffer(buffer_allocator, vertices->buffer,
                             vertices->memory);
            this->vertices.remove(rc->handle);
            break;
        }

        case RenderResourceType::INDEX: {
            HardwareIndexVk *index = this->indices.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(index);
            this->destroy_queue.push(DestroyResource{
                .type = RenderResourceType::INDEX,
                .buffer = {.buffer = index->buffer, .memory = index->memory}});
            vmaDestroyBuffer(buffer_allocator, index->buffer, index->memory);
            this->indices.remove(rc->handle);
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureVk *tex = this->textures.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(tex);
            this->destroy_queue.push(
                DestroyResource{.type = RenderResourceType::TEXTURE,
                                .texture = {.image = tex->image,
                                            .view = tex->view,
                                            .sampler = tex->sampler,
                                            .memory = tex->memory}});
            vkDestroySampler(device, tex->sampler, nullptr);
            vkDestroyImageView(device, tex->view, nullptr);
            vmaDestroyImage(buffer_allocator, tex->image, tex->memory);
            this->textures.remove(rc->handle);
            break;
        }
        case RenderResourceType::SHADER: {
            HardwareShaderVk *shader = this->shaders.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(shader);
            this->destroy_queue.push(
                DestroyResource{.type = RenderResourceType::SHADER,
                                .shader = {.set_layout = shader->set_layout,
                                           .layout = shader->layout}});
            vkDestroyDescriptorSetLayout(device, shader->set_layout, nullptr);
            vkDestroyPipelineLayout(device, shader->layout, nullptr);
            this->shaders.remove(rc->handle);
            break;
        }
        case RenderResourceType::PIPELINE: {
            this->pipelines.remove(rc->handle);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetVk *rt =
                this->render_targets.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(rt);
            this->destroy_queue.push(DestroyResource{
                .type = RenderResourceType::VERTEX,
                .render_target = {.render_pass = rt->render_pass_cache,
                                  .framebuffer = rt->framebuffer_cache}});
            vkDestroyRenderPass(device, rt->render_pass_cache, nullptr);
            vkDestroyFramebuffer(device, rt->framebuffer_cache, nullptr);
            this->render_targets.remove(rc->handle);
            break;
        }
        default:
            break;
    }
}

void RenderBackendVK::process_commands(std::deque<RenderCommand> &cmd_queue) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    while (!buffer_copy_queue.empty()) {
        BufferCopy &copy = buffer_copy_queue.front();
        VkBufferCopy _copy{};
        _copy.srcOffset = 0;
        _copy.dstOffset = 0;
        _copy.size = copy.size;
        vkCmdCopyBuffer(command_buffer, copy.staging_buffer, copy.target_buffer,
                        1, &_copy);
        buffer_copy_queue.pop();
    }

    while (!image_copy_queue.empty()) {
        ImageCopy &copy = image_copy_queue.front();
        VkBufferImageCopy _copy{};
        _copy.bufferOffset = 0;
        _copy.bufferRowLength = 0;
        _copy.bufferImageHeight = 0;

        _copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        _copy.imageSubresource.mipLevel = 0;
        _copy.imageSubresource.baseArrayLayer = 0;
        _copy.imageSubresource.layerCount = 1;

        _copy.imageOffset = {0, 0, 0};
        _copy.imageExtent = {copy.w, copy.h, 1};
        vkCmdCopyBufferToImage(command_buffer, copy.staging_buffer,
                               copy.target_image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_copy);
        image_copy_queue.pop();
    }

    RenderCommandType last_type;
    bool render_pass_begin = false;
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderPass = current_render_target->render_pass_cache;
    renderPassInfo.framebuffer = current_render_target->framebuffer_cache;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = VkExtent2D{
        .width = current_render_target->w, .height = current_render_target->h};

    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                if (render_pass_begin) {
                    vkCmdEndRenderPass(command_buffer);
                    render_pass_begin = false;
                }
                vkCmdEndRenderPass(command_buffer);
                handle_update(cmd);
                break;
            case RenderCommandType::STATE:
                if (render_pass_begin) {
                    vkCmdEndRenderPass(command_buffer);
                    render_pass_begin = false;
                }
                handle_state(cmd);
                break;
            case RenderCommandType::RENDER:
                if (!render_pass_begin) {
                    vkCmdBeginRenderPass(command_buffer, &renderPassInfo,
                                         VK_SUBPASS_CONTENTS_INLINE);
                    render_pass_begin = true;
                }

                handle_render(cmd);
                break;
            case RenderCommandType::BEGIN_SCOPE:
                // glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1,
                //                  (const GLchar *)cmd.data);
                break;
            case RenderCommandType::END_SCOPE:
                //
                break;
            default:
                break;
        }
        cmd_queue.pop_front();
    }
    if (render_pass_begin) {
        vkCmdEndRenderPass(command_buffer);
        render_pass_begin = false;
    }
    vkEndCommandBuffer(command_buffer);
    // vkQueueSubmit();
    while (!destroy_queue.empty()) {
        DestroyResource &destroy = destroy_queue.front();
        switch (destroy.type) {
            case RenderResourceType::VERTEX:
            case RenderResourceType::INDEX:
            case RenderResourceType::CONSTANT:
            case RenderResourceType::BUFFER:
                vmaDestroyBuffer(buffer_allocator, destroy.buffer.buffer,
                                 destroy.buffer.memory);
                /* code */
                break;
            case RenderResourceType::SHADER:
                vkDestroyDescriptorSetLayout(device, destroy.shader.set_layout,
                                             nullptr);
                vkDestroyPipelineLayout(device, destroy.shader.layout, nullptr);
                break;
            case RenderResourceType::TEXTURE:
                vkDestroySampler(device, destroy.texture.sampler, nullptr);
                vkDestroyImageView(device, destroy.texture.view, nullptr);
                vmaDestroyImage(buffer_allocator, destroy.texture.image,
                                destroy.texture.memory);
                break;
            case RenderResourceType::RENDER_TARGET:
                vkDestroyRenderPass(device, destroy.render_target.render_pass,
                                    nullptr);
                vkDestroyFramebuffer(device, destroy.render_target.framebuffer,
                                     nullptr);
                break;
            default:
                break;
        }
        destroy_queue.pop();
    }
}

void RenderBackendVK::swap_buffer() {}

void RenderBackendVK::handle_update(RenderCommand &cmd) {
    RenderUpdateData *update_data = static_cast<RenderUpdateData *>(cmd.data);
    if (!update_data->filled) {
        this->push_cmd(cmd);
        return;
    }
    RenderResource rc = update_data->rc;
    /* data is right after header */
    void *data = update_data->get_buffer();
    switch (rc.type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferVk *vertex = this->vertices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(vertex);
            if (vertex->frequence == UpdateFrequence::IMMUTABLE) {
                break;
            }
            if (vertex->size <
                update_data->buffer.offset + update_data->buffer.size) {
                reallocate_buffer(
                    vertex, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                    update_data->buffer.offset + update_data->buffer.size);
            }
            vkCmdUpdateBuffer(command_buffer, vertex->buffer,
                              update_data->buffer.offset,
                              update_data->buffer.size, data);

            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexVk *index = this->indices.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(index);
            if (index->frequence == UpdateFrequence::IMMUTABLE) {
                break;
            }
            if (index->size <
                update_data->buffer.offset + update_data->buffer.size) {
                reallocate_buffer(
                    index, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    update_data->buffer.offset + update_data->buffer.size);
            }
            vkCmdUpdateBuffer(command_buffer, index->buffer,
                              update_data->buffer.offset,
                              update_data->buffer.size, data);
            break;
        }
        case RenderResourceType::TEXTURE: {
            HardwareTextureVk *tex = this->textures.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(tex);
            VkBuffer staging_buffer;
            VmaAllocation staging_allocation;
            auto _texture = update_data->texture;
            u64 size = _texture.w * _texture.h *
                       VulkanHelper::texture_format(tex->format);
            create_staging_buffer(&staging_buffer, &staging_allocation, size);
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {(i32)_texture.x_off, (i32)_texture.y_off, 0};
            region.imageExtent = {_texture.w, _texture.h, 1};
            vkCmdCopyBufferToImage(command_buffer, staging_buffer, tex->image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &region);
            this->destroy_queue.push(
                DestroyResource{.type = RenderResourceType::VERTEX,
                                .buffer = {.buffer = staging_buffer,
                                           .memory = staging_allocation}});
            break;
        }
        case RenderResourceType::CONSTANT: {
            HardwareBufferVk *constant = this->constants.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(constant);
            if (constant->frequence == UpdateFrequence::IMMUTABLE) {
                break;
            }
            u64 offset =
                std::min(constant->size, (u64)update_data->buffer.offset);
            u64 size = std::min(constant->size - offset,
                                (u64)update_data->buffer.size);
            vkCmdUpdateBuffer(command_buffer, constant->buffer, offset, size,
                              data);
            break;
        }

        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetVk *rt =
                this->render_targets.get_or_null(rc.handle);
            EXPECT_NOT_NULL_BREAK(rt);
            HardwareTextureVk *tex = this->textures.get_or_null(
                update_data->attachment.texture.handle);
            EXPECT_NOT_NULL_BREAK(tex);
            VkFormat format = VulkanHelper::texture_format(tex->format);
            /* we'll replace same slot or replace depth attachment */
            bool flag = false;
            for (u32 i = 0; i < rt->attachments.size(); i++) {
                HardwareAttachmentVk &attchement = rt->attachments[i];
                if (attchement.is_depth && update_data->attachment.is_depth ||
                    attchement.slot && update_data->attachment.slot) {
                    if (attchement.image_format != format) {
                        attchement.image_format = format;
                        rt->dirty = true;
                    }
                    attchement.texture_handle =
                        update_data->attachment.texture.handle;
                    flag = true;
                }
            }

            /* not found so we insert*/
            if (!flag) {
                rt->attachments.push_back(HardwareAttachmentVk{
                    .slot = update_data->attachment.slot,
                    .is_depth = update_data->attachment.is_depth,
                    .image_format = format,
                    .texture_handle = update_data->attachment.texture.handle});
                rt->dirty = true;
            }

            break;
        }
        case RenderResourceType::BUFFER: {
            HardwareBufferVk *buffer = this->ssbos.get_or_null(rc.handle);
            EXPECT_NOT_NULL_RET(buffer);
            if (buffer->frequence == UpdateFrequence::IMMUTABLE) {
                break;
            }
            u64 offset =
                std::min(buffer->size, (u64)update_data->buffer.offset);
            u64 size =
                std::min(buffer->size - offset, (u64)update_data->buffer.size);
            vkCmdUpdateBuffer(command_buffer, buffer->buffer, offset, size,
                              data);
            break;
        }
        default:
            break;
    }
    free(update_data);
}

void RenderBackendVK::handle_state(RenderCommand &cmd) {
    RenderStateData *state_data = static_cast<RenderStateData *>(cmd.data);
    RenderStateData::Operation *head =
        (RenderStateData::Operation *)(((u64)state_data) +
                                       sizeof(RenderStateData));
    bool viewport_set = false;
    bool scissor_set = false;
    VkViewport viewport_rect;
    VkRect2D scissor_rect;
    std::vector<VkClearAttachment> attachments;
    VkClearRect clear_rect{};

    for (i32 i = 0; i < state_data->operation_cnt; i++) {
        auto *op = &head[i];
        auto type = op->type;
        switch (type) {
            case RenderStateData::OpType::CLEAR: {
                if (op->clear_flag & CLEAR_COLOR) {
                    attachments.emplace_back(VkClearAttachment{
                        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                        .colorAttachment = 0,
                        .clearValue = VkClearValue{.color = {0, 0, 0, 1}}});
                }
                if (op->clear_flag & CLEAR_DEPTH) {
                    attachments.emplace_back(VkClearAttachment{
                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .clearValue =
                            VkClearValue{.depthStencil = {.depth = 0}}});
                }
                if (op->clear_flag & CLEAR_STENCIL) {
                    attachments.emplace_back(VkClearAttachment{
                        .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                        .clearValue =
                            VkClearValue{.depthStencil = {.stencil = 0}}});
                }
                break;
            }
            case RenderStateData::OpType::VIEWPORT: {
                viewport_rect.x = op->viewports.view_rects[0].x;
                viewport_rect.y = op->viewports.view_rects[0].y;
                viewport_rect.width = op->viewports.view_rects[0].w;
                viewport_rect.height = op->viewports.view_rects[0].h;
                viewport_set = true;
                break;
            }
            case RenderStateData::OpType::SCISSOR: {
                scissor_rect.offset.x = op->scissor_rect.x;
                scissor_rect.offset.y = op->scissor_rect.y;
                scissor_rect.extent.width = op->scissor_rect.w;
                scissor_rect.extent.height = op->scissor_rect.h;
                scissor_set = true;
                break;
            }
            case RenderStateData::OpType::BIND_RENDER_TARGET: {
                HardwareRenderTargetVk *target_rt = nullptr;

                target_rt =
                    this->render_targets.get_or_null(op->render_target.handle);
                if (target_rt) {
                    current_render_target = target_rt;
                }
                break;
            }
            case RenderStateData::OpType::BIND_BUFFERBASE: {
                break;
            }
            default:
                break;
        }
    }

    clear_rect.rect.extent.width = current_render_target->w;
    clear_rect.rect.extent.height = current_render_target->h;
    if (scissor_set) {
        clear_rect.rect = scissor_rect;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor_rect);
    }
    if (viewport_set) {
        vkCmdSetViewport(command_buffer, 0, 1, &viewport_rect);
    }
    if (!attachments.empty()) {
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = current_render_target->render_pass_cache;
        renderPassInfo.framebuffer = current_render_target->framebuffer_cache;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent =
            VkExtent2D{.width = current_render_target->w,
                       .height = current_render_target->h};

        vkCmdBeginRenderPass(command_buffer, &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);

        vkCmdClearAttachments(command_buffer, attachments.size(),
                              attachments.data(), 1, &clear_rect);

        vkCmdEndRenderPass(command_buffer);
    }
}

void RenderBackendVK::handle_render(RenderCommand &cmd) {
    RenderDrawData *draw_data = static_cast<RenderDrawData *>(cmd.data);
    RenderDrawData::Operation *head =
        (RenderDrawData::Operation *)(((u64)draw_data) +
                                      sizeof(RenderDrawData));
    u32 index_type = 0;
    std::vector<VkBuffer> vertex;
    VkBuffer index = nullptr;

    std::vector<VertexLayout *> layouts;

    for (i32 i = 0; i < draw_data->operation_cnt; i++) {
        auto *op = &head[i];
        auto type = op->type;
        switch (type) {
            case RenderDrawData::OpType::BIND_VERTEX: {
                HardwareBufferVk *_vertex =
                    this->vertices.get_or_null(op->vertex_rc.handle);
                vertex.push_back(_vertex->buffer);
                break;
            }
            case RenderDrawData::OpType::BIND_INDEX: {
                HardwareIndexVk *_index =
                    this->indices.get_or_null(op->index_rc.handle);
                index = _index->buffer;
                break;
            }
            case RenderDrawData::OpType::BIND_DESC:
                layouts.push_back(op->vertex_desc);
                break;
            case RenderDrawData::OpType::BIND_TEXTURE:
                break;
            case RenderDrawData::OpType::VIEWPORT: {
                VkViewport viewport_rect;
                viewport_rect.x = op->view_rect.x;
                viewport_rect.y = op->view_rect.y;
                viewport_rect.width = op->view_rect.w;
                viewport_rect.height = op->view_rect.h;
                vkCmdSetViewport(command_buffer, 0, 1, &viewport_rect);
                break;
            }
            case RenderDrawData::OpType::SCISSOR: {
                VkRect2D scissor_rect;
                scissor_rect.offset.x = op->scissor_rect.x;
                scissor_rect.offset.y = op->scissor_rect.y;
                scissor_rect.extent.width = op->scissor_rect.w;
                scissor_rect.extent.height = op->scissor_rect.h;
                vkCmdSetScissor(command_buffer, 0, 1, &scissor_rect);
                break;
            }
            case RenderDrawData::OpType::PUSH_CONSTANT: {
                break;
            }
            default:
                break;
        }
    }
    create_framebuffer(current_render_target);
    HardwarePipelineVk *pipeline =
        this->pipelines.get_or_null(draw_data->pipeline.handle);
    VkPipeline vk_pipeline =
        get_vk_pipeline(pipeline, current_render_target, layouts);
    VkDeviceSize offsets[] = {0};
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vk_pipeline);

    vkCmdBindVertexBuffers(command_buffer, 0, vertex.size(), vertex.data(),
                           offsets);
    if (index) {
        vkCmdBindIndexBuffer(command_buffer, index, 0,
                             VkIndexType::VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(command_buffer, draw_data->vertex_cnt,
                         draw_data->instance_cnt, draw_data->index_offset,
                         draw_data->vertex_offset, draw_data->instance_offset);
    } else {
        vkCmdDraw(command_buffer, draw_data->vertex_cnt,
                  draw_data->instance_cnt, draw_data->vertex_offset,
                  draw_data->instance_offset);
    }
}

void RenderBackendVK::use_texture(u32 unit, RenderResource &rc) {}

}  // namespace Seed