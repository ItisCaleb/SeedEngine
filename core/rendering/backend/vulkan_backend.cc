#include "vulkan_backend.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>
#include <cstddef>
#include <vector>
#include <set>
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_resource.h"
#define VOLK_IMPLEMENTATION
#include <volk.h>
#define VMA_IMPLEMENTATION
#ifdef __APPLE__
#include <vk_mem_alloc.h>
#else
#include <vma/vk_mem_alloc.h>
#endif
#include "core/math/utils.h"
#include "core/macro.h"
#include "core/misc/hash.h"
#include <utility>

namespace Seed {

static const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

static const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_MAINTENANCE_1_EXTENSION_NAME};

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
    current_window = window;
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
        requiredExtensions.emplace_back(
            VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }
    VkValidationFeatureEnableEXT validation_features[] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    };
    VkValidationFeaturesEXT validation_features_info{};
    validation_features_info.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validation_features_info.enabledValidationFeatureCount =
        sizeof(validation_features) / sizeof(validation_features[0]);
    validation_features_info.pEnabledValidationFeatures = validation_features;

    createInfo.enabledExtensionCount = requiredExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (enable_validation) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
        createInfo.pNext = &validation_features_info;
        spdlog::debug("Enabling Vulkan validation layer");
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
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

    VkPhysicalDeviceVulkan12Features features12{};
    features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    features12.separateDepthStencilLayouts = true;
    features12.descriptorBindingPartiallyBound = true;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &features12;
    deviceFeatures2.features.tessellationShader = true;
    deviceFeatures2.features.samplerAnisotropy = true;
    deviceFeatures2.features.independentBlend = true;
    deviceFeatures2.features.depthClamp = true;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pNext = &deviceFeatures2;
    createInfo.pEnabledFeatures = nullptr;
    createInfo.enabledExtensionCount = deviceExtensions.size();
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

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
    if (target_extent.height == 0 || target_extent.width == 0) return;

    /* clamp image count */
    u32 imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 &&
        imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainKHR old_swapchain = swap_chain.chain;

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
    createInfo.oldSwapchain = old_swapchain;
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

void RenderBackendVK::recreate_swapchain(Window *window) {
    vkDeviceWaitIdle(device);

    for (u32 i = 0; i < swap_chain.render_targets.size(); i++) {
        HardwareTextureVk *tex = textures.get_or_null(swap_chain.textures[i]);
        /* set image to null to prevent vma destroy it */
        tex->image = nullptr;
        dealloc(RenderResourceType::TEXTURE, swap_chain.textures[i]);

        if (i > 0) {
            HardwareRenderPassVk *rt =
                this->render_pass.get_or_null(swap_chain.render_targets[i]);
            rt->render_pass_cache = nullptr;
        }
        dealloc(RenderResourceType::RENDER_TARGET,
                swap_chain.render_targets[i]);
    }

    swap_chain.textures.clear();
    swap_chain.render_targets.clear();
    create_swapchain(window);
    create_image_views();
    create_swapchain_framebuffer();
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
        HardwareRenderPassVk rt;
        rt.is_swapchain = true;
        rt.clear_flag = StateClearFlagBits::CLEAR_COLOR |
                        StateClearFlagBits::CLEAR_DEPTH |
                        StateClearFlagBits::CLEAR_STENCIL;
        rt.color_attachments.push_back(HardwareColorAttachmentVk{
            .slot = 0,
            .image_format = this->swap_chain.format,
            .texture_handle = this->swap_chain.textures[i]});

        /* we let the swap chain to share render pass*/
        if (i == 0) {
            create_render_pass(&rt);
            render_pass = rt.render_pass_cache;
        } else {
            rt.render_pass_cache = render_pass;
            rt.dirty = false;
        }
        create_framebuffer(&rt);
        Handle handle = this->render_pass.insert(rt);
        this->swap_chain.render_targets.push_back(handle);
    }
    current_render_target = NULL_HANDLE;
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

    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (vkAllocateCommandBuffers(device, &allocInfo,
                                     &frames[i].render_cmd_buffer) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }
}

void RenderBackendVK::create_descriptor_pool() {
    VkDescriptorPoolSize ssbos_dynamic{};
    VkDescriptorPoolSize ubos_dynamic{};
    VkDescriptorPoolSize ssbos{};
    VkDescriptorPoolSize ubos{};
    VkDescriptorPoolSize samplers{};
    ssbos.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssbos.descriptorCount = 16;
    ubos.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubos.descriptorCount = 50;
    ssbos_dynamic.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    ssbos_dynamic.descriptorCount = 16;
    ubos_dynamic.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    ubos_dynamic.descriptorCount = 50;
    samplers.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplers.descriptorCount = 500;
    std::vector<VkDescriptorPoolSize> size = {ssbos, ubos, ssbos_dynamic,
                                              ubos_dynamic, samplers};

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = size.size();
    poolInfo.pPoolSizes = size.data();
    poolInfo.maxSets = 1000;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptor_pool) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void RenderBackendVK::create_sync_objects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (u32 i = 0; i < swap_chain.render_targets.size(); i++) {
        VkSemaphore semaphore;
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore) !=
            VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
        swap_chain.semaphore.push_back(semaphore);
    }

    for (u32 i = 0; i < FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                              &frames[i].image_available_semaphore) !=
                VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr,
                          &frames[i].in_flight_fence) != VK_SUCCESS) {
            throw std::runtime_error("failed to create semaphores!");
        }
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
                                                 VkBufferUsageFlags usage,
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

    if (vmaCreateBuffer(buffer_allocator, &bufferInfo, &allocInfo, buffer,
                        allocation, nullptr) != VK_SUCCESS) {
        SPDLOG_ERROR("Failed to allocate CPU visible buffer.");
    }

    if (data) {
        vmaCopyMemoryToAllocation(buffer_allocator, data, *allocation, 0,
                                  bufferInfo.size);
    }
}

void RenderBackendVK::create_gpu_only_buffer(VkBuffer *buffer,
                                             VmaAllocation *allocation,
                                             VkBufferUsageFlags usage, u64 size,
                                             const void *data) {
    /* create target buffer */
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = size;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;

    if (vmaCreateBuffer(buffer_allocator, &bufferInfo, &allocInfo, buffer,
                        allocation, nullptr) != VK_SUCCESS) {
        SPDLOG_ERROR("Failed to allocate GPU only buffer.");
    }

    /* copy to dedicated memory later */
    if (data) {
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
        vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                                  size);
        this->static_buffer_update_queue.push(
            StaticBufferUpdate{.staging_buffer = stagingBuffer,
                               .staging_allocation = stagingAllocation,
                               .target_buffer = *buffer,
                               .size = size});
    }
}

inline VkImageMemoryBarrier RenderBackendVK::create_image_barrier(
    HardwareTextureVk *texture, VkImageLayout target_layout, u32 layer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = texture->layouts[layer];
    barrier.newLayout = target_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->image;
    barrier.srcAccessMask =
        VulkanHelper::access_mask_for_layout(barrier.oldLayout);
    barrier.dstAccessMask =
        VulkanHelper::access_mask_for_layout(barrier.newLayout);
    barrier.subresourceRange.aspectMask =
        VulkanHelper::aspect_flag(texture->format);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = layer;
    barrier.subresourceRange.layerCount = 1;

    texture->layouts[layer] = target_layout;
    return barrier;
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

void RenderBackendVK::push_buffer_update(HardwareBufferVk *buffer, u64 offset,
                                         u64 size, void *data) {
    if (buffer->size < offset + size) {
        reallocate_buffer(buffer, offset + size);
    }
    /* static buffer */
    if (buffer->mapped_ptr == nullptr) {
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
        vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                                  size);
        this->static_buffer_update_queue.push(
            StaticBufferUpdate{.staging_buffer = stagingBuffer,
                               .staging_allocation = stagingAllocation,
                               .target_buffer = buffer->buffer,
                               .offset = offset,
                               .size = size});
        free(data);

    } else {
        this->dynamic_buffer_update_queue.push(
            DynamicBufferUpdate{.data = data,
                                .target_buffer = buffer->mapped_ptr,
                                .allocation = buffer->memory,
                                .offset = offset,
                                .size = size});
    }
}

void RenderBackendVK::push_image_update(TextureHandle handle, u32 layer,
                                        u32 offx, u32 offy, u32 w, u32 h,
                                        void *data) {
    HardwareTextureVk *texture = this->textures.get_or_null(handle);
    u32 pixel_size = get_pixel_format_size(texture->format);
    u32 size = w * h * pixel_size;
    if (texture->w * texture->h * pixel_size <
        size + offx * offy * pixel_size) {
        SPDLOG_ERROR("Texture reallocation is not supported");
        return;
    }
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
    vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                              size);
    free(data);
    this->image_copy_queue.push(
        ImageUpdate{.staging_buffer = stagingBuffer,
                    .staging_allocation = stagingAllocation,
                    .texture = handle,
                    .face = layer,
                    .offx = offx,
                    .offy = offy,
                    .w = w,
                    .h = h});
}

void RenderBackendVK::push_buffer_destroy(RenderResourceType type,
                                          VkBuffer buffer,
                                          VmaAllocation allocation,
                                          bool mapped) {
    this->destroy_list.push_back(
        DestroyResource{.type = type,
                        .mapped = mapped,
                        .buffer = {.buffer = buffer, .memory = allocation}});
}

void RenderBackendVK::reallocate_buffer(HardwareBufferVk *buffer, u64 size) {
    VkBuffer new_buffer;
    VmaAllocation allocation;

    /* create new buffer */
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.usage = buffer->usage;

    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.size = size;
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                      VMA_ALLOCATION_CREATE_MAPPED_BIT;
    vmaCreateBuffer(buffer_allocator, &bufferInfo, &allocInfo, &new_buffer,
                    &allocation, nullptr);

    if (buffer->mapped_ptr != nullptr) {
        vmaUnmapMemory(buffer_allocator, buffer->memory);
    }
    push_buffer_destroy(RenderResourceType::VERTEX, buffer->buffer,
                        buffer->memory, false);

    /* rebind */
    buffer->size = size;
    buffer->buffer = new_buffer;
    buffer->memory = allocation;
    if (buffer->mapped_ptr != nullptr) {
        vmaMapMemory(buffer_allocator, buffer->memory, &buffer->mapped_ptr);
    }
}

void RenderBackendVK::stream_buffer(HardwareBufferVk *buffer, u64 size,
                                    u64 alignment, void *data) {
    Frame &frame = frames[get_current_frame_index()];
    u64 aligned_size = (size + alignment - 1) & ~(alignment - 1);

    if (buffer->tail + aligned_size > buffer->size) {
        if (aligned_size < buffer->head) {
            /* check if there is empty space at start */
            vmaCopyMemoryToAllocation(buffer_allocator, data, buffer->memory, 0,
                                      size);
            u64 waste = buffer->size - buffer->tail;
            buffer->current = 0;
            buffer->tail = aligned_size;
            frame.usages.push_back(Frame::StreamBufferUsage{
                .buffer = buffer, .size = waste + aligned_size});
            return;
        } else {
            /* else reallocate */
            reallocate_buffer(buffer, (buffer->tail + aligned_size) * 2);
        }
    } else if (buffer->head > buffer->tail &&
               buffer->tail + aligned_size > buffer->head) {
        /* we append data to end */
        buffer->tail = buffer->size + buffer->tail;
        reallocate_buffer(buffer, buffer->size * 2);
    }
    vmaCopyMemoryToAllocation(buffer_allocator, data, buffer->memory,
                              buffer->tail, size);
    buffer->current = buffer->tail;
    buffer->tail += aligned_size;
    frame.usages.push_back(
        Frame::StreamBufferUsage{.buffer = buffer, .size = aligned_size});
}

VkDescriptorSet RenderBackendVK::get_descriptor_set(
    DescriptorSetLayout *layout, std::vector<Binding> &bindings,
    bool is_global) {
    Hash _hash;

    for (auto &binding : bindings) {
        _hash.update(&binding.binding_point);
        _hash.update(&binding.type);
        _hash.update(&binding.resource_id);
    }
    u64 hash = _hash.digest();
    auto iter = descriptor_set_cache.find(hash);
    if (iter != descriptor_set_cache.end()) {
        return iter->second;
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptor_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout->vk_layout;

    VkDescriptorSet set{};
    if (vkAllocateDescriptorSets(device, &allocInfo, &set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;

    writes.reserve(layout->set.bindings.size());
    bufferInfos.reserve(layout->set.bindings.size());
    imageInfos.reserve(layout->set.bindings.size());

    for (ShaderBinding &shader_binding : layout->set.bindings) {
        auto it = std::find_if(bindings.begin(), bindings.end(), [&](auto &b) {
            return b.binding_point == shader_binding.binding_point;
        });

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = shader_binding.binding_point;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;

        if (shader_binding.type == ShaderResourceType::UBO) {
            HardwareBufferVk *constant = nullptr;
            if (it != bindings.end())
                constant = this->constants.get_or_null(it->handle);
            if (!constant) continue;

            VkDescriptorBufferInfo info{};
            info.buffer = constant->buffer;
            info.offset = 0;
            info.range = shader_binding.size;
            bufferInfos.push_back(info);

            write.descriptorType =
                is_global ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            write.pBufferInfo = &bufferInfos.back();
        } else if (shader_binding.type == ShaderResourceType::SSBO) {
            HardwareBufferVk *storage_buffer = nullptr;
            if (it != bindings.end())
                storage_buffer = this->ssbos.get_or_null(it->handle);
            if (!storage_buffer) continue;

            VkDescriptorBufferInfo info{};
            info.buffer = storage_buffer->buffer;
            info.offset = 0;
            info.range = VK_WHOLE_SIZE;
            bufferInfos.push_back(info);

            write.descriptorType =
                is_global ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
                          : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            write.pBufferInfo = &bufferInfos.back();
        } else if (shader_binding.type == ShaderResourceType::SAMPLER) {
            HardwareTextureVk *texture = nullptr;
            if (it != bindings.end())
                texture = this->textures.get_or_null(it->handle);
            if (!texture) continue;

            VkDescriptorImageInfo info{};
            info.imageView = texture->view;
            info.sampler = texture->sampler;
            info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos.push_back(info);

            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pImageInfo = &imageInfos.back();
        }

        writes.push_back(write);
    }

    vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);
    descriptor_set_cache.emplace(hash, set);
    return set;
}

void RenderBackendVK::bind_descriptor_set(HardwareShaderVk *shader, u32 binding,
                                          std::vector<Binding> &bindings,
                                          bool is_global) {
    Frame &frame = frames[get_current_frame_index()];

    VkDescriptorSet set =
        get_descriptor_set(shader->set_layouts[binding], bindings, is_global);
    std::vector<u32> offsets;
    for (ShaderBinding &binding : shader->set_layouts[binding]->set.bindings) {
        auto it = std::find_if(bindings.begin(), bindings.end(), [&](auto &b) {
            return b.binding_point == binding.binding_point;
        });
        if (binding.type == ShaderResourceType::UBO) {
            HardwareBufferVk *constant = nullptr;
            if (it != bindings.end())
                constant = this->constants.get_or_null(it->handle);
            if (!constant) continue;
            if (!is_global) offsets.push_back(constant->current);
        } else if (binding.type == ShaderResourceType::SSBO) {
            HardwareBufferVk *storage_buffer = nullptr;
            if (it != bindings.end())
                storage_buffer = this->ssbos.get_or_null(it->handle);
            if (!storage_buffer) continue;
            if (!is_global) offsets.push_back(storage_buffer->current);
        }
    }

    vkCmdBindDescriptorSets(frame.render_cmd_buffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, shader->layout,
                            binding, 1, &set, offsets.size(), offsets.data());
}

TextureHandle RenderBackendVK::create_texture(TextureType type, u32 w, u32 h,
                                              PixelFormat format, u32 count,
                                              MSAAType msaa_type,
                                              const SamplerProperty &property,
                                              bool should_map) {
    VkImage image;
    VkImageView image_view;
    VkSampler sampler;
    VkImageCreateInfo imageInfo{};
    VkImage msaa_image = nullptr;
    VkImageView msaa_view = nullptr;

    bool is_depth = VulkanHelper::is_depth(format);
    bool is_stencil = VulkanHelper::is_stencil(format);
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (is_stencil || is_depth) {
        imageInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    } else {
        imageInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.format = VulkanHelper::texture_format(format);
    imageInfo.imageType = VulkanHelper::texture_type(type);
    imageInfo.extent = VkExtent3D{.width = w, .height = h, .depth = 1};
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.mipLevels = 1;
    std::vector<VkImageLayout> layouts;
    if (type == TextureType::TEXTURE_CUBEMAP) {
        imageInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        count = 6;
    }

    imageInfo.arrayLayers = count;
    for (u32 i = 0; i < count; i++) {
        layouts.push_back(VK_IMAGE_LAYOUT_UNDEFINED);
    }
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;

    if (should_map) {
        imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
        allocInfo.flags =
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    }

    VmaAllocation allocation;
    VmaAllocation msaa_allocation = nullptr;
    vmaCreateImage(buffer_allocator, &imageInfo, &allocInfo, &image,
                   &allocation, nullptr);

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.format = imageInfo.format;

    viewInfo.subresourceRange.aspectMask = VulkanHelper::aspect_flag(format);
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

    if (msaa_type != MSAAType::SAMPLE_COUNT_1) {
        imageInfo.samples = VulkanHelper::sample_count(msaa_type);
        imageInfo.mipLevels = 1;
        if (is_stencil || is_depth) {
            imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        } else {
            imageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        vmaCreateImage(buffer_allocator, &imageInfo, &allocInfo, &msaa_image,
                       &msaa_allocation, nullptr);
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.image = msaa_image;
        vkCreateImageView(device, &viewInfo, nullptr, &msaa_view);
    }
    TextureHandle handle = this->textures.insert({
        .w = w,
        .h = h,
        .type = type,
        .format = format,
        .image = image,
        .view = image_view,
        .sampler = sampler,
        .memory = allocation,
        .sample_count = imageInfo.samples,
        .msaa_image = msaa_image,
        .msaa_view = msaa_view,
        .msaa_memory = msaa_allocation,
        .msaa_layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .layouts = layouts,
    });

    /* prevent image without data doesn't transition */
    for (u32 i = 0; i < count; i++) {
        image_transition_queue.push(std::make_pair(handle, i));
    }
    return handle;
}

TextureHandle RenderBackendVK::alloc_texture(TextureType type, u32 w, u32 h,
                                             PixelFormat format,
                                             MSAAType msaa_type,
                                             const SamplerProperty &property,
                                             const void *data) {
    TextureHandle handle =
        create_texture(type, w, h, format, 1, msaa_type, property, false);

    /* upload using staging buffer */
    if (data) {
        size_t size = w * h * get_pixel_format_size(format);
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
        vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                                  size);
        this->image_copy_queue.push(ImageUpdate{
            .staging_buffer = stagingBuffer,
            .staging_allocation = stagingAllocation,
            .texture = handle,
            .face = 0,
            .offx = 0,
            .offy = 0,
            .w = w,
            .h = h,
        });
    }
    return handle;
}

TextureHandle RenderBackendVK::alloc_textures(TextureType type, u32 w, u32 h,
                                              PixelFormat format, u32 count,
                                              const SamplerProperty &property) {
    TextureHandle handle = create_texture(
        type, w, h, format, count, MSAAType::SAMPLE_COUNT_1, property, false);

    return handle;
}

TextureHandle RenderBackendVK::alloc_cubemap(u32 w, u32 h, PixelFormat format,
                                             const SamplerProperty &property) {
    TextureHandle handle =
        create_texture(TextureType::TEXTURE_CUBEMAP, w, h, format, 6,
                       MSAAType::SAMPLE_COUNT_1, property, false);

    return handle;
}

TextureHandle RenderBackendVK::alloc_mappable_texture(
    TextureType type, u32 w, u32 h, PixelFormat format,
    const SamplerProperty &property, const void *data) {
    if (type != TextureType::TEXTURE_1D && type != TextureType::TEXTURE_2D &&
        type != TextureType::TEXTURE_3D) {
        SPDLOG_ERROR("Can't create mappable texture as target type");
        return NULL_HANDLE;
    }
    TextureHandle handle = create_texture(
        type, w, h, format, 1, MSAAType::SAMPLE_COUNT_1, property, true);
    HardwareTextureVk *texture = this->textures.get_or_null(handle);

    if (data) {
        size_t size = w * h * get_pixel_format_size(format);
        vmaCopyMemoryToAllocation(buffer_allocator, data, texture->memory, 0,
                                  size);
    }
    void *mapped_ptr;
    vmaMapMemory(buffer_allocator, texture->memory, &mapped_ptr);

    /* query texture real size */
    VkImageSubresource subRes = {};
    subRes.aspectMask = VulkanHelper::aspect_flag(format);
    subRes.mipLevel = 0;
    subRes.arrayLayer = 0;

    VkSubresourceLayout subResLayout;
    vkGetImageSubresourceLayout(device, texture->image, &subRes, &subResLayout);
    w = subResLayout.rowPitch / get_pixel_format_size(format);
    texture->w = w;
    texture->mapped_ptr = mapped_ptr;
    image_transition_queue.push(std::make_pair(handle, 0));
    return handle;
}

void RenderBackendVK::update_texture_sampler(TextureHandle handle, u32 layer,
                                             const SamplerProperty &property) {
    HardwareTextureVk *tex = this->textures.get_or_null(handle);
    EXPECT_NOT_NULL_RET(tex);

    VkSamplerCreateInfo samplerInfo = VulkanHelper::sampler_info(
        property, device_properties.limits.maxSamplerAnisotropy);
    this->destroy_list.push_back(
        DestroyResource{.type = RenderResourceType::TEXTURE,
                        .texture = {
                            .image = nullptr,
                            .view = nullptr,
                            .sampler = tex->sampler,
                            .memory = nullptr,
                        }});
    if (vkCreateSampler(device, &samplerInfo, nullptr, &tex->sampler) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}
void RenderBackendVK::query_texture_size(TextureHandle handle, u32 *w, u32 *h) {
    HardwareTextureVk *tex = this->textures.get_or_null(handle);
    EXPECT_NOT_NULL_RET(tex);
    *w = tex->w;
    *h = tex->h;
}

VertexHandle RenderBackendVK::alloc_vertex(u32 stride, u32 element_cnt,
                                           UpdateFrequence frequence,
                                           const void *data) {
    VkBuffer vertex;
    VmaAllocation allocation;
    /* if no element then just allocate */
    /* a big buffer to minimize reallocation*/
    if (element_cnt == 0) {
        element_cnt = 10000;
    }
    void *mapped_ptr = nullptr;
    if (frequence == UpdateFrequence::STATIC) {
        create_gpu_only_buffer(&vertex, &allocation,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               stride * element_cnt, data);
    } else {
        create_host_visible_buffer(&vertex, &allocation,
                                   VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                   stride * element_cnt, data);
        vmaMapMemory(buffer_allocator, allocation, &mapped_ptr);
    }

    return this->vertices.insert({.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                  .buffer = vertex,
                                  .memory = allocation,
                                  .frequence = frequence,
                                  .size = stride * element_cnt,
                                  .mapped_ptr = mapped_ptr});
}

IndexHandle RenderBackendVK::alloc_indices(IndexType type, u32 element_cnt,
                                           UpdateFrequence frequence,
                                           const void *data) {
    VkBuffer indice;
    VmaAllocation allocation;
    /* if no element then just allocate */
    /* a big buffer to minimize reallocation*/
    if (element_cnt == 0) {
        element_cnt = 10000;
    }
    void *mapped_ptr = nullptr;
    u64 size = get_index_size(type) * element_cnt;
    if (frequence == UpdateFrequence::STATIC) {
        create_gpu_only_buffer(&indice, &allocation,
                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size, data);
    } else {
        create_host_visible_buffer(
            &indice, &allocation, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size, data);
        vmaMapMemory(buffer_allocator, allocation, &mapped_ptr);
    }

    HardwareIndexVk index;
    index.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    index.type = type;
    index.buffer = indice;
    index.size = size;
    index.frequence = frequence;
    index.memory = allocation;
    index.mapped_ptr = mapped_ptr;
    return this->indices.insert(index);
}

ConstantHandle RenderBackendVK::alloc_constant(u32 size, const void *data,
                                               UpdateFrequence frequence) {
    VkBuffer constant;
    VmaAllocation allocation;
    void *mapped_ptr = nullptr;
    if (frequence == UpdateFrequence::STATIC) {
        create_gpu_only_buffer(&constant, &allocation,
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, data);

    } else {
        create_host_visible_buffer(&constant, &allocation,
                                   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size,
                                   data);
        vmaMapMemory(buffer_allocator, allocation, &mapped_ptr);
    }
    return this->constants.insert({.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                   .buffer = constant,
                                   .memory = allocation,
                                   .frequence = frequence,
                                   .size = size,
                                   .mapped_ptr = mapped_ptr});
}

SSBOHandle RenderBackendVK::alloc_storage_buffer(u32 size, const void *data,
                                                 UpdateFrequence frequence) {
    VkBuffer buffer;
    VmaAllocation allocation;
    void *mapped_ptr = nullptr;
    if (frequence == UpdateFrequence::STATIC) {
        create_gpu_only_buffer(&buffer, &allocation,
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size, data);

    } else {
        create_host_visible_buffer(&buffer, &allocation,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size,
                                   data);

        vmaMapMemory(buffer_allocator, allocation, &mapped_ptr);
    }

    return this->ssbos.insert({.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               .buffer = buffer,
                               .memory = allocation,
                               .frequence = frequence,
                               .size = size,
                               .mapped_ptr = mapped_ptr});
}

VkShaderModule RenderBackendVK::create_shader_module(const KString &shader) {
    VkShaderModule module = nullptr;
    if (shader.is_empty()) return module;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shader.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(shader.data());
    vkCreateShaderModule(device, &createInfo, nullptr, &module);
    return module;
}

ShaderHandle RenderBackendVK::alloc_shader(const KString &vertex_code,
                                           const KString &fragment_code,
                                           const KString &geometry_code,
                                           const KString &tess_ctrl_code,
                                           const KString &tess_eval_code) {
    return shaders.insert(HardwareShaderVk{.vertex_src = vertex_code,
                                           .geo_src = geometry_code,
                                           .tess_ctrl_src = tess_ctrl_code,
                                           .tess_eval_src = tess_eval_code,
                                           .fragment_src = fragment_code,
                                           .layout = nullptr});
}

void RenderBackendVK::setup_shader_layout(ShaderHandle handle,
                                          const ShaderLayout &shader_layout) {
    HardwareShaderVk *shader = this->shaders.get_or_null(handle);
    EXPECT_NOT_NULL_RET(shader);
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    std::vector<VkPushConstantRange> ranges;
    std::vector<VkDescriptorBindingFlags> binding_flags;

    std::vector<DescriptorSetLayout *> set_layouts;
    /* set 0: global, set 1: local*/
    i32 set_idx = -1;
    for (const ShaderBindingSet &set : shader_layout.get_binding_sets()) {
        set_idx++;
        VkDescriptorSetLayout set_layout;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        Hash _hash;
        for (const ShaderBinding &binding : set.bindings) {
            VkDescriptorSetLayoutBinding _binding{};
            _binding.binding = binding.binding_point;
            _binding.descriptorCount = binding.count;
            _binding.descriptorType =
                VulkanHelper::descriptor_type(binding.type, set_idx == 1);
            _binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
            _hash.update(&_binding.binding);
            _hash.update(&_binding.descriptorCount);
            _hash.update(&_binding.descriptorType);
            binding_flags.push_back(VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);

            bindings.push_back(_binding);
        }
        u64 hash = _hash.digest();
        auto iter = descriptor_layout_cache.find(hash);
        if (iter != descriptor_layout_cache.end()) {
            set_layouts.push_back(&iter->second);
            continue;
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo flagInfo{};
        flagInfo.sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        flagInfo.bindingCount = bindings.size();
        flagInfo.pBindingFlags = binding_flags.data();

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();
        layoutInfo.pNext = &flagInfo;

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                        &set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        auto _iter = descriptor_layout_cache.emplace(
            hash, DescriptorSetLayout{.vk_layout = set_layout, .set = set});
        set_layouts.push_back(&_iter.first->second);
    }
    /* bindings */

    u32 i = 0;
    for (const PushConstantRange &range : shader_layout.get_push_contants()) {
        VkPushConstantRange _range;
        _range.offset = range.offset;
        _range.size = range.size;
        _range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        ranges.push_back(_range);
        i++;
    }
    std::vector<VkDescriptorSetLayout> vk_set_layouts;
    for (DescriptorSetLayout *layout : set_layouts) {
        vk_set_layouts.push_back(layout->vk_layout);
    };

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = vk_set_layouts.size();
    pipelineLayoutInfo.pSetLayouts = vk_set_layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
    pipelineLayoutInfo.pPushConstantRanges = ranges.data();
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan pipeline layout!");
    }
    shader->set_layouts = set_layouts;
    shader->layout = pipelineLayout;
}

PipelineHandle RenderBackendVK::alloc_pipeline(
    ShaderHandle shader, const RenderRasterizerState &rst_state,
    const RenderDepthStencilState &depth_state,
    const RenderBlendState &blend_state) {
    HardwareShaderVk *_shader = shaders.get_or_null(shader);
    if (!_shader) {
        throw std::runtime_error(
            "failed to create Vulkan pipeline layout! Shader is null.");
    }

    HardwarePipelineVk pipeline;
    pipeline.shader = shader;
    pipeline.rst_state = rst_state;
    pipeline.depth_state = depth_state;
    pipeline.blend_attachment = blend_state;
    return this->pipelines.insert(pipeline);
}

void RenderBackendVK::create_render_pass(HardwareRenderPassVk *render_target) {
    VkSubpassDescription subpass{};
    std::vector<VkAttachmentDescription> allAttachments;
    std::vector<VkAttachmentReference> colorRefs;
    VkAttachmentReference depthRef{};
    std::vector<VkAttachmentReference> resolveRefs;
    u32 i = 0;

    if (!render_target->dirty) {
        return;
    }
    render_target->dirty = false;
    bool is_swapchain = render_target->is_swapchain;

    if (render_target->render_pass_cache != nullptr) {
        this->destroy_list.push_back(
            DestroyResource{.type = RenderResourceType::RENDER_TARGET,
                            .render_target = {
                                .render_pass = render_target->render_pass_cache,
                                .framebuffer = nullptr,
                            }});
    }

    for (auto attachment : render_target->color_attachments) {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = attachment.image_format;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = (render_target->clear_flag & CLEAR_COLOR)
                                     ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                     : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout =
            is_swapchain ? VK_IMAGE_LAYOUT_UNDEFINED
                         : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout =
            is_swapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                         : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        allAttachments.push_back(colorAttachment);

        if (render_target->sample_count != VK_SAMPLE_COUNT_1_BIT) {
            VkAttachmentReference resolveRef;
            resolveRef.attachment = i;
            resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolveRefs.push_back(resolveRef);
            i++;

            /* msaa attachment */
            colorAttachment.samples = render_target->sample_count;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            allAttachments.push_back(colorAttachment);
        }
        VkAttachmentReference colorRef;
        colorRef.attachment = i;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorRefs.push_back(colorRef);
        i++;
    }
    if (render_target->depth_attachment.texture_handle != NULL_HANDLE) {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = render_target->depth_attachment.image_format;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = (render_target->clear_flag & CLEAR_DEPTH)
                                     ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                     : VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp =
            (render_target->clear_flag & CLEAR_STENCIL)
                ? VK_ATTACHMENT_LOAD_OP_CLEAR
                : VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.initialLayout =
            (render_target->depth_attachment.is_depth &&
             render_target->depth_attachment.is_stencil)
                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : render_target->depth_attachment.is_depth
                ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.finalLayout = depthAttachment.initialLayout;

        allAttachments.push_back(depthAttachment);
        if (render_target->sample_count != VK_SAMPLE_COUNT_1_BIT) {
            VkAttachmentReference resolveRef;
            resolveRef.attachment = i;
            resolveRef.layout = depthAttachment.finalLayout;
            resolveRefs.push_back(resolveRef);
            i++;

            /* msaa attachment */
            depthAttachment.samples = render_target->sample_count;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            allAttachments.push_back(depthAttachment);
        }
        depthRef.attachment = i;
        depthRef.layout = depthAttachment.finalLayout;
        subpass.pDepthStencilAttachment = &depthRef;
        i++;
    }
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = colorRefs.size();
    subpass.pColorAttachments = colorRefs.data();
    subpass.pResolveAttachments = resolveRefs.data();

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
        is_swapchain ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                     : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstStageMask =
        is_swapchain ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                     : VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    VkAccessFlags normal_flag = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.srcAccessMask = is_swapchain ? 0 : normal_flag;
    dependency.dstAccessMask = is_swapchain
                                   ? (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                      VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)
                                   : normal_flag;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = allAttachments.size();
    renderPassInfo.pAttachments = allAttachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr,
                           &render_target->render_pass_cache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan render pass!");
    }
}

void RenderBackendVK::create_framebuffer(HardwareRenderPassVk *render_target) {
    if (!render_target->texture_changed) {
        return;
    }
    render_target->texture_changed = false;

    if (render_target->framebuffer_cache != nullptr) {
        this->destroy_list.push_back(
            DestroyResource{.type = RenderResourceType::RENDER_TARGET,
                            .render_target = {
                                .render_pass = nullptr,
                                .framebuffer = render_target->framebuffer_cache,
                            }});
    }

    VkFramebufferCreateInfo framebufferInfo{};
    std::vector<VkImageView> attachments;
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = render_target->render_pass_cache;
    framebufferInfo.layers = 1;
    u32 width = 0;
    u32 height = 0;
    for (auto &attachment : render_target->color_attachments) {
        HardwareTextureVk *texture =
            this->textures.get_or_null(attachment.texture_handle);
        attachments.push_back(texture->view);
        width = std::max(texture->w, framebufferInfo.width);
        height = std::max(texture->h, framebufferInfo.height);
        if (render_target->sample_count != VK_SAMPLE_COUNT_1_BIT) {
            attachments.push_back(texture->msaa_view);
        }
    }
    if (render_target->depth_attachment.texture_handle != NULL_HANDLE) {
        HardwareTextureVk *texture = this->textures.get_or_null(
            render_target->depth_attachment.texture_handle);
        attachments.push_back(texture->view);
        width = std::max(texture->w, framebufferInfo.width);
        height = std::max(texture->h, framebufferInfo.height);
        if (render_target->sample_count != VK_SAMPLE_COUNT_1_BIT) {
            attachments.push_back(texture->msaa_view);
        }
    }
    framebufferInfo.width = width;
    framebufferInfo.height = height;
    framebufferInfo.attachmentCount = attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    render_target->w = width;
    render_target->h = height;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &render_target->framebuffer_cache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan framebuffer!");
    }
}

HardwareRenderPassVk *RenderBackendVK::get_current_render_pass() {
    if (current_render_target == NULL_HANDLE) {
        return this->render_pass.get_or_null(
            this->swap_chain.render_targets[swap_chain.next_index]);
    } else {
        return this->render_pass.get_or_null(current_render_target);
    }
}

VkPipeline RenderBackendVK::get_vk_pipeline(
    HardwarePipelineVk *pipeline, HardwareRenderPassVk *render_target,
    std::vector<VertexLayout *> &layouts, VkPrimitiveTopology primitive,
    VkSampleCountFlagBits sample_count, bool draw_depth_only,
    bool depth_clamp) {
    Hash _hash;
    _hash.update(&pipeline->rst_state.cull_mode);
    _hash.update(&pipeline->rst_state.poly_mode);
    _hash.update(&pipeline->rst_state.patch_control_points);

    _hash.update(&pipeline->depth_state.stencil_on);
    _hash.update(&pipeline->depth_state.stencil_compare_op);

    _hash.update(&pipeline->blend_attachment.blend_on);
    _hash.update(&pipeline->blend_attachment.func.src_rgb);
    _hash.update(&pipeline->blend_attachment.func.dst_rgb);
    _hash.update(&pipeline->blend_attachment.func.src_alpha);
    _hash.update(&pipeline->blend_attachment.func.dst_alpha);
    _hash.update(&primitive);
    _hash.update(&sample_count);
    _hash.update(&draw_depth_only);
    _hash.update(&depth_clamp);
    i32 shader_id = shaders.get_id(pipeline->shader);
    _hash.update(&shader_id);

    /* since we usually don't create multiple render target/layout with
     * same config */
    /* we just hash its handle and address here*/
    _hash.update(&render_target->render_pass_cache);
    for (auto layout : layouts) {
        _hash.update(&layout);
    }
    u64 hash = _hash.digest();
    VkPipeline vk_pipeline;
    auto iter = pipeline_cache.find(hash);
    if (iter == pipeline_cache.end()) {
        vk_pipeline =
            create_vk_pipeline(pipeline, render_target, layouts, primitive,
                               sample_count, draw_depth_only, depth_clamp);
        pipeline_cache.emplace(hash, vk_pipeline);
    } else {
        vk_pipeline = iter->second;
    }
    return vk_pipeline;
}

std::vector<VkClearValue> RenderBackendVK::get_clear_values(
    HardwareRenderPassVk *rp) {
    std::vector<VkClearValue> clears;
    for (u32 i = 0; i < rp->color_attachments.size(); i++) {
        clears.emplace_back(VkClearValue{.color = {0, 0, 0, 1}});
    }
    if (rp->depth_attachment.texture_handle != NULL_HANDLE) {
        clears.emplace_back(
            VkClearValue{.depthStencil = {.depth = 1, .stencil = 1}});
    }
    return clears;
}

void RenderBackendVK::handle_frame_begin() {
    std::vector<VkImageMemoryBarrier> transfer_barriers;
    std::vector<VkImageMemoryBarrier> shader_barriers;
    std::vector<VkBufferMemoryBarrier> buffer_barriers;

    Frame &frame = frames[get_current_frame_index()];
    transfer_barriers.reserve(image_copy_queue.size());
    shader_barriers.reserve(image_copy_queue.size() +
                            image_transition_queue.size());

    std::set<std::pair<u32, u32>> copied;
    for (auto it = image_copy_queue.rbegin(); it != image_copy_queue.rend();
         ++it) {
        ImageUpdate &copy = *it;
        /* make sure only copy once every frame */
        if (copied.count({copy.texture, copy.face}) > 0) {
            copy.texture = NULL_HANDLE;
            continue;
        };
        copied.insert({copy.texture, copy.face});

        VkImageMemoryBarrier barrier{};
        HardwareTextureVk *texture = this->textures.get_or_null(copy.texture);

        barrier = create_image_barrier(
            texture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copy.face);
        transfer_barriers.push_back(barrier);

        barrier = create_image_barrier(
            texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, copy.face);
        shader_barriers.push_back(barrier);
    }
    while (!image_transition_queue.is_empty()) {
        auto tex = image_transition_queue.peek();
        TextureHandle handle = tex.first;
        u32 face = tex.second;
        HardwareTextureVk *texture = this->textures.get_or_null(handle);
        image_transition_queue.pop();
        if (texture->layouts[face] == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            continue;
        shader_barriers.push_back(create_image_barrier(
            texture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, face));
    }

    vkCmdPipelineBarrier(
        frame.render_cmd_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
        transfer_barriers.size(), transfer_barriers.data());

    while (!static_buffer_update_queue.is_empty()) {
        StaticBufferUpdate &copy = static_buffer_update_queue.peek();
        VkBufferCopy _copy{};
        _copy.srcOffset = 0;
        _copy.dstOffset = copy.offset;
        _copy.size = copy.size;
        vkCmdCopyBuffer(frame.render_cmd_buffer, copy.staging_buffer,
                        copy.target_buffer, 1, &_copy);
        VkBufferMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.buffer = copy.target_buffer;
        barrier.offset = copy.offset;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask =
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT |
            VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        barrier.size = copy.size;
        buffer_barriers.push_back(barrier);
        this->destroy_list.push_back(
            DestroyResource{.type = RenderResourceType::VERTEX,
                            .mapped = false,
                            .buffer = {.buffer = copy.staging_buffer,
                                       .memory = copy.staging_allocation}});
        static_buffer_update_queue.pop();
    }

    while (!dynamic_buffer_update_queue.is_empty()) {
        DynamicBufferUpdate &copy = dynamic_buffer_update_queue.peek();
        memcpy((void *)((u64)copy.target_buffer + copy.offset), copy.data,
               copy.size);
        free(copy.data);
        vmaFlushAllocation(buffer_allocator, copy.allocation, copy.offset,
                           copy.size);
        dynamic_buffer_update_queue.pop();
    }
    while (!image_copy_queue.is_empty()) {
        ImageUpdate &copy = image_copy_queue.peek();
        HardwareTextureVk *texture = this->textures.get_or_null(copy.texture);
        if (texture) {
            VkBufferImageCopy _copy{};
            _copy.bufferOffset = 0;
            _copy.bufferRowLength = 0;
            _copy.bufferImageHeight = 0;

            _copy.imageSubresource.aspectMask =
                VulkanHelper::aspect_flag(texture->format);
            _copy.imageSubresource.mipLevel = 0;
            _copy.imageSubresource.baseArrayLayer = copy.face;
            _copy.imageSubresource.layerCount = 1;

            _copy.imageOffset = {(i32)copy.offx, (i32)copy.offy, 0};
            _copy.imageExtent = {copy.w, copy.h, 1};
            vkCmdCopyBufferToImage(
                frame.render_cmd_buffer, copy.staging_buffer, texture->image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_copy);
        }

        this->destroy_list.push_back(
            DestroyResource{.type = RenderResourceType::VERTEX,
                            .mapped = false,
                            .buffer = {.buffer = copy.staging_buffer,
                                       .memory = copy.staging_allocation}});
        image_copy_queue.pop();
    }
    vkCmdPipelineBarrier(frame.render_cmd_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr,
                         buffer_barriers.size(), buffer_barriers.data(),
                         shader_barriers.size(), shader_barriers.data());
}

void RenderBackendVK::handle_frame_end() {
    std::vector<VkImageMemoryBarrier> transfer_barriers;
    std::vector<VkImageMemoryBarrier> shader_barriers;

    Frame &frame = frames[get_current_frame_index()];
    transfer_barriers.reserve(image_blit_queue.size());
    shader_barriers.reserve(image_blit_queue.size() +
                            image_transition_queue.size());

    std::set<std::pair<u32, u32>> blited;
    for (auto it = image_blit_queue.rbegin(); it != image_blit_queue.rend();
         ++it) {
        ImageBlit &blit = *it;
        /* make sure only copy once every frame */
        if (blited.count({blit.dst, blit.dst_layer}) > 0) {
            blit.dst = NULL_HANDLE;
            continue;
        };
        blited.insert({blit.dst, blit.dst_layer});
        VkImageMemoryBarrier barrier{};
        HardwareTextureVk *dst = this->textures.get_or_null(blit.dst);
        if (!dst) {
            continue;
        }
        barrier = create_image_barrier(
            dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blit.dst_layer);
        transfer_barriers.push_back(barrier);

        barrier = create_image_barrier(
            dst, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, blit.dst_layer);
        shader_barriers.push_back(barrier);
    }
    for (auto it = image_blit_queue.rbegin(); it != image_blit_queue.rend();
         ++it) {
        ImageBlit &blit = *it;
        /* we only need to transition once for source */
        if (blited.count({blit.src, blit.src_layer}) > 0) {
            continue;
        };
        blited.insert({blit.src, blit.src_layer});
        VkImageMemoryBarrier barrier{};
        HardwareTextureVk *src = this->textures.get_or_null(blit.src);
        if (!src) {
            continue;
        }
        barrier = create_image_barrier(
            src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, blit.src_layer);
        transfer_barriers.push_back(barrier);

        barrier = create_image_barrier(
            src, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, blit.src_layer);
        shader_barriers.push_back(barrier);
    }

    vkCmdPipelineBarrier(
        frame.render_cmd_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
        transfer_barriers.size(), transfer_barriers.data());

    while (!image_blit_queue.is_empty()) {
        ImageBlit &blit = image_blit_queue.peek();
        HardwareTextureVk *dst = this->textures.get_or_null(blit.dst);
        HardwareTextureVk *src = this->textures.get_or_null(blit.src);
        if (dst && src) {
            VkImageBlit image_blit{};
            image_blit.dstOffsets[0] = {blit.dst_region.x, blit.dst_region.y,
                                        0};
            image_blit.dstOffsets[1] = {blit.dst_region.w, blit.dst_region.h,
                                        1};
            image_blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_blit.dstSubresource.mipLevel = 0;
            image_blit.dstSubresource.baseArrayLayer = blit.dst_layer;
            image_blit.dstSubresource.layerCount = 1;
            image_blit.srcOffsets[0] = {blit.src_region.x, blit.src_region.y,
                                        0};
            image_blit.srcOffsets[1] = {blit.src_region.w, blit.src_region.h,
                                        1};
            image_blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_blit.srcSubresource.mipLevel = 0;
            image_blit.srcSubresource.baseArrayLayer = blit.src_layer;
            image_blit.srcSubresource.layerCount = 1;
            VkFilter filter = src->format != PixelFormat::RGBA
                                  ? VK_FILTER_NEAREST
                                  : VK_FILTER_LINEAR;
            vkCmdBlitImage(frame.render_cmd_buffer, src->image,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit,
                           filter);
        }
        image_blit_queue.pop();
    }
    vkCmdPipelineBarrier(
        frame.render_cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0, nullptr,
        shader_barriers.size(), shader_barriers.data());
}

void RenderBackendVK::handle_destroy() {
    for (auto it = destroy_list.begin(); it != destroy_list.end();) {
        DestroyResource &destroy = *it;
        if (destroy.frame_count < FRAMES_IN_FLIGHT) {
            destroy.frame_count++;
            it++;
            continue;
        }
        switch (destroy.type) {
            case RenderResourceType::VERTEX:
            case RenderResourceType::INDEX:
            case RenderResourceType::CONSTANT:
            case RenderResourceType::STORAGE_BUFFER:
                if (destroy.mapped) {
                    vmaUnmapMemory(buffer_allocator, destroy.buffer.memory);
                }
                vmaDestroyBuffer(buffer_allocator, destroy.buffer.buffer,
                                 destroy.buffer.memory);
                break;
            case RenderResourceType::SHADER:
                /* Since layouts might be reused, we won't destroy them. */
                vkDestroyPipelineLayout(device, destroy.shader.layout, nullptr);
                break;
            case RenderResourceType::TEXTURE:
                if (destroy.mapped) {
                    vmaUnmapMemory(buffer_allocator, destroy.texture.memory);
                }
                if (destroy.texture.sampler != nullptr) {
                    vkDestroySampler(device, destroy.texture.sampler, nullptr);
                }
                if (destroy.texture.view != nullptr) {
                    vkDestroyImageView(device, destroy.texture.view, nullptr);
                }
                if (destroy.texture.image != nullptr) {
                    vmaDestroyImage(buffer_allocator, destroy.texture.image,
                                    destroy.texture.memory);
                }
                break;
            case RenderResourceType::RENDER_TARGET:
                if (destroy.render_target.framebuffer) {
                    vkDestroyFramebuffer(
                        device, destroy.render_target.framebuffer, nullptr);
                }
                if (destroy.render_target.render_pass) {
                    vkDestroyRenderPass(
                        device, destroy.render_target.render_pass, nullptr);
                }

                break;
            default:
                break;
        }
        it = destroy_list.erase(it);
    }
}

void RenderBackendVK::transition_render_pass(HardwareRenderPassVk *rt,
                                             bool to_attachment) {
    /* render pass already handled swapchain transition for us*/
    if (rt->is_swapchain) {
        return;
    }
    Frame &frame = frames[get_current_frame_index()];
    VkImageLayout target_color_layout =
        to_attachment ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
                      : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkImageLayout target_depth_layout;
    if (to_attachment) {
        target_depth_layout =
            (rt->depth_attachment.is_stencil && rt->depth_attachment.is_depth)
                ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            : rt->depth_attachment.is_depth
                ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                : VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
    } else {
        target_depth_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    std::vector<VkImageMemoryBarrier> barriers;
    for (HardwareColorAttachmentVk &attachment : rt->color_attachments) {
        HardwareTextureVk *tex =
            this->textures.get_or_null(attachment.texture_handle);
        /* we'll only transition msaa for first time*/
        if (tex->msaa_image != nullptr &&
            tex->msaa_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
            VkImageMemoryBarrier msaa_barrier{};
            msaa_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            msaa_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            msaa_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            msaa_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            msaa_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            msaa_barrier.image = tex->msaa_image;
            msaa_barrier.subresourceRange.aspectMask =
                VulkanHelper::aspect_flag(tex->format);
            msaa_barrier.subresourceRange.baseMipLevel = 0;
            msaa_barrier.subresourceRange.levelCount = 1;
            msaa_barrier.subresourceRange.baseArrayLayer = 0;
            msaa_barrier.subresourceRange.layerCount = 1;
            tex->msaa_layout = msaa_barrier.newLayout;
            barriers.push_back(msaa_barrier);
        }
        barriers.push_back(create_image_barrier(tex, target_color_layout, 0));
    }
    if (rt->depth_attachment.texture_handle != NULL_HANDLE) {
        HardwareTextureVk *depth_tex =
            this->textures.get_or_null(rt->depth_attachment.texture_handle);
        if (depth_tex->msaa_image != nullptr &&
            depth_tex->msaa_layout == VK_IMAGE_LAYOUT_UNDEFINED) {
            VkImageMemoryBarrier msaa_barrier{};
            msaa_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            msaa_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            msaa_barrier.newLayout =
                (rt->depth_attachment.is_stencil &&
                 rt->depth_attachment.is_depth)
                    ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                : rt->depth_attachment.is_depth
                    ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            msaa_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            msaa_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            msaa_barrier.image = depth_tex->msaa_image;
            msaa_barrier.subresourceRange.aspectMask =
                VulkanHelper::aspect_flag(depth_tex->format);
            msaa_barrier.subresourceRange.baseMipLevel = 0;
            msaa_barrier.subresourceRange.levelCount = 1;
            msaa_barrier.subresourceRange.baseArrayLayer = 0;
            msaa_barrier.subresourceRange.layerCount = 1;
            depth_tex->msaa_layout = msaa_barrier.newLayout;
            barriers.push_back(msaa_barrier);
        }
        barriers.push_back(
            create_image_barrier(depth_tex, target_depth_layout, 0));
    }
    vkCmdPipelineBarrier(frame.render_cmd_buffer,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0,
                         nullptr, barriers.size(), barriers.data());
}

VkPipeline RenderBackendVK::create_vk_pipeline(
    HardwarePipelineVk *pipeline, HardwareRenderPassVk *render_target,
    std::vector<VertexLayout *> &layouts, VkPrimitiveTopology primitive,
    VkSampleCountFlagBits sample_count, bool draw_depth_only,
    bool depth_clamp) {
    VkPipeline graphicsPipeline;
    HardwareShaderVk *shader = this->shaders.get_or_null(pipeline->shader);
    if (!shader) {
        throw std::runtime_error(
            "failed to create graphics pipeline! No shader provided");
    }
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,           VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY, VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE, VK_DYNAMIC_STATE_DEPTH_COMPARE_OP};

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

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
    assemblyInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyInfo.topology = primitive;
    assemblyInfo.primitiveRestartEnable = false;

    VkPipelineRasterizationStateCreateInfo rasterState =
        VulkanHelper::rasterizer(pipeline->rst_state);
    VkPipelineDepthStencilStateCreateInfo depthState =
        VulkanHelper::depth_stencil(pipeline->depth_state);
    rasterState.depthClampEnable = depth_clamp;

    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
    for (HardwareColorAttachmentVk &attchment :
         render_target->color_attachments) {
        VkPipelineColorBlendAttachmentState state =
            VulkanHelper::blend_attachment(pipeline->blend_attachment);
        if (attchment.image_format == VK_FORMAT_R16G16B16A16_SINT) {
            state.blendEnable = false;
        }
        blendAttachments.push_back(state);
    }
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    colorBlending.attachmentCount = blendAttachments.size();
    colorBlending.pAttachments = blendAttachments.data();
    colorBlending.blendConstants[0] = 0.0f;  // Optional
    colorBlending.blendConstants[1] = 0.0f;  // Optional
    colorBlending.blendConstants[2] = 0.0f;  // Optional
    colorBlending.blendConstants[3] = 0.0f;  // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = sample_count;
    multisampling.minSampleShading = 1.0f;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    VkPipelineTessellationStateCreateInfo tessellation{};
    tessellation.sType =
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    tessellation.patchControlPoints = pipeline->rst_state.patch_control_points;

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
    if (frag && !draw_depth_only) {
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
    vertexInputInfo.vertexBindingDescriptionCount = binding_desc.size();

    /* create pipeline */
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = stageInfos.size();
    pipelineInfo.pStages = stageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pInputAssemblyState = &assemblyInfo;
    pipelineInfo.pRasterizationState = &rasterState;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pTessellationState = &tessellation;
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

RenderPassHandle RenderBackendVK::alloc_render_pass() {
    return this->render_pass.insert({});
}

void RenderBackendVK::blit_texture(TextureHandle dst, TextureHandle src,
                                   u32 dst_layer, u32 src_layer,
                                   const Rect &dst_region,
                                   const Rect &src_region) {
    this->image_blit_queue.push({.dst = dst,
                                 .src = src,
                                 .dst_layer = dst_layer,
                                 .src_layer = src_layer,
                                 .dst_region = dst_region,
                                 .src_region = src_region});
}

void RenderBackendVK::update(RenderResourceType type, Handle handle, u32 offset,
                             u32 size, void *data) {
    HardwareBufferVk *buffer = nullptr;
    switch (type) {
        case RenderResourceType::VERTEX:
            buffer = this->vertices.get_or_null(handle);
            break;
        case RenderResourceType::INDEX:
            buffer = this->indices.get_or_null(handle);
            break;
        case RenderResourceType::CONSTANT:
            buffer = this->constants.get_or_null(handle);
            break;
        case RenderResourceType::STORAGE_BUFFER:
            buffer = this->ssbos.get_or_null(handle);
            break;
        default:
            break;
    }
    EXPECT_NOT_NULL_RET(buffer);
    push_buffer_update(buffer, offset, size, data);
}

void RenderBackendVK::update(TextureHandle handle, u32 layer, u32 offx,
                             u32 offy, u32 w, u32 h, void *data) {
    HardwareTextureVk *texture = this->textures.get_or_null(handle);
    EXPECT_NOT_NULL_RET(texture);
    push_image_update(handle, layer, offx, offy, w, h, data);
}

void *RenderBackendVK::map_buffer(RenderResourceType type, Handle handle) {
    HardwareBufferVk *buffer = nullptr;
    switch (type) {
        case RenderResourceType::VERTEX:
            buffer = this->vertices.get_or_null(handle);
            break;
        case RenderResourceType::INDEX:
            buffer = this->indices.get_or_null(handle);
            break;
        case RenderResourceType::CONSTANT:
            buffer = this->constants.get_or_null(handle);
            break;
        case RenderResourceType::STORAGE_BUFFER:
            buffer = this->ssbos.get_or_null(handle);
            break;
        default:
            break;
    }
    EXPECT_NOT_NULL_RET(buffer, nullptr);
    if (buffer->mapped_ptr == nullptr) {
        SPDLOG_ERROR("Trying to map unmappable buffer '{}'", (Handle)handle);
    }
    return buffer->mapped_ptr;
}

void *RenderBackendVK::map_texture(TextureHandle handle) {
    HardwareTextureVk *texture = this->textures.get_or_null(handle);
    EXPECT_NOT_NULL_RET(texture, nullptr);
    if (texture->mapped_ptr == nullptr) {
        SPDLOG_ERROR("Trying to map unmappable texture '{}'", (Handle)handle);
    }
    return texture->mapped_ptr;
}

void RenderBackendVK::bind_depth_attachment(RenderPassHandle handle,
                                            TextureHandle texture, u32 face) {
    HardwareRenderPassVk *rt = this->render_pass.get_or_null(handle);
    EXPECT_NOT_NULL_RET(rt);
    HardwareTextureVk *tex = this->textures.get_or_null(texture);
    EXPECT_NOT_NULL_RET(tex);
    /* TODO: check all attachment is same sample count */
    rt->sample_count = tex->sample_count;
    VkFormat format = VulkanHelper::texture_format(tex->format);
    rt->depth_attachment = HardwareDepthStencilAttachmentVk{
        .is_depth = VulkanHelper::is_depth(tex->format),
        .is_stencil = VulkanHelper::is_stencil(tex->format),
        .image_format = format,
        .texture_handle = texture};
}

void RenderBackendVK::bind_color_attachment(RenderPassHandle handle, u8 slot,
                                            TextureHandle texture, u32 face) {
    HardwareRenderPassVk *rt = this->render_pass.get_or_null(handle);
    EXPECT_NOT_NULL_RET(rt);
    HardwareTextureVk *tex = this->textures.get_or_null(texture);
    EXPECT_NOT_NULL_RET(tex);
    VkFormat format = VulkanHelper::texture_format(tex->format);
    /* TODO: check all attachment is same sample count */
    rt->sample_count = tex->sample_count;
    /* we'll replace same slot or replace depth attachment */
    bool flag = false;
    for (u32 i = 0; i < rt->color_attachments.size(); i++) {
        HardwareColorAttachmentVk &attachment = rt->color_attachments[i];
        if (attachment.slot == slot) {
            if (attachment.image_format != format) {
                attachment.image_format = format;
                rt->dirty = true;
            }
            attachment.texture_handle = texture;
            rt->texture_changed = true;
            return;
        }
    }
    rt->color_attachments.push_back(HardwareColorAttachmentVk{
        .slot = slot, .image_format = format, .texture_handle = texture});
    rt->dirty = true;
    rt->texture_changed = true;
}

void RenderBackendVK::dealloc(RenderResourceType type, Handle handle) {
    HardwareBufferVk *_buffer;
    HardwareTextureVk *_tex;
    HardwareShaderVk *_shader;
    HardwareRenderPassVk *_rt;
    switch (type) {
        case RenderResourceType::VERTEX:
            _buffer = this->vertices.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_buffer);
            push_buffer_destroy(type, _buffer->buffer, _buffer->memory,
                                _buffer->mapped_ptr != nullptr);
            this->vertices.remove(handle);
            break;
        case RenderResourceType::INDEX:
            _buffer = this->indices.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_buffer);
            push_buffer_destroy(type, _buffer->buffer, _buffer->memory,
                                _buffer->mapped_ptr != nullptr);
            this->indices.remove(handle);
            break;
        case RenderResourceType::CONSTANT:
            _buffer = this->constants.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_buffer);
            push_buffer_destroy(type, _buffer->buffer, _buffer->memory,
                                _buffer->mapped_ptr != nullptr);
            this->constants.remove(handle);
            break;
        case RenderResourceType::STORAGE_BUFFER:
            _buffer = this->ssbos.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_buffer);
            push_buffer_destroy(type, _buffer->buffer, _buffer->memory,
                                _buffer->mapped_ptr != nullptr);
            this->ssbos.remove(handle);
            break;
        case RenderResourceType::TEXTURE:
            _tex = this->textures.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_tex);
            this->destroy_list.push_back(
                DestroyResource{.type = type,
                                .mapped = (_tex->mapped_ptr != nullptr),
                                .texture = {.image = _tex->image,
                                            .view = _tex->view,
                                            .sampler = _tex->sampler,
                                            .memory = _tex->memory}});
            if (_tex->msaa_image != nullptr) {
                this->destroy_list.push_back(
                    DestroyResource{.type = type,
                                    .mapped = false,
                                    .texture = {.image = _tex->msaa_image,
                                                .view = _tex->msaa_view,
                                                .sampler = nullptr,
                                                .memory = _tex->msaa_memory}});
            }
            this->textures.remove(handle);
            break;
        case RenderResourceType::PIPELINE:
            this->pipelines.remove(handle);
            break;
        case RenderResourceType::SHADER:
            _shader = this->shaders.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_shader);
            this->destroy_list.push_back(DestroyResource{
                .type = type, .shader = {.layout = _shader->layout}});
            this->shaders.remove(handle);
            break;
        case RenderResourceType::RENDER_TARGET:
            _rt = this->render_pass.get_or_null(handle);
            EXPECT_NOT_NULL_RET(_rt);
            this->destroy_list.push_back(DestroyResource{
                .type = type,
                .render_target = {.render_pass = _rt->render_pass_cache,
                                  .framebuffer = _rt->framebuffer_cache}});
            this->render_pass.remove(handle);
            break;
        default:
            break;
    }
}

void RenderBackendVK::process_commands(std::deque<RenderCommand> &cmd_queue) {
    should_present = false;

    Frame &frame = frames[get_current_frame_index()];

    vkWaitForFences(device, 1, &frame.in_flight_fence, VK_TRUE, UINT64_MAX);
    // VmaTotalStatistics stats;
    // vmaCalculateStatistics(buffer_allocator, &stats);
    // spdlog::debug("VMA usage total : {} bytes",
    // stats.total.statistics.allocationBytes);
    VkResult result = vkAcquireNextImageKHR(
        device, swap_chain.chain, UINT64_MAX, frame.image_available_semaphore,
        VK_NULL_HANDLE, &swap_chain.next_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreate_swapchain(current_window);
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    vkResetFences(device, 1, &frame.in_flight_fence);

    vkResetCommandBuffer(frame.render_cmd_buffer, 0);

    /* consume ring buffer */
    for (Frame::StreamBufferUsage &usage : frame.usages) {
        usage.buffer->head += usage.size;
        usage.buffer->head %= usage.buffer->size;
    }
    frame.usages.clear();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    if (vkBeginCommandBuffer(frame.render_cmd_buffer, &beginInfo) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    /* handle copy, transition etc... */
    handle_frame_begin();

    HardwareRenderPassVk *rt = get_current_render_pass();
    create_render_pass(rt);
    create_framebuffer(rt);
    std::vector<VkClearValue> clears = get_clear_values(rt);
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.clearValueCount = clears.size();
    renderPassInfo.pClearValues = clears.data();
    renderPassInfo.renderPass = rt->render_pass_cache;
    renderPassInfo.framebuffer = rt->framebuffer_cache;
    renderPassInfo.renderArea.extent =
        VkExtent2D{.width = rt->w, .height = rt->h};

    vkCmdBeginRenderPass(frame.render_cmd_buffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                handle_update(cmd);
                break;
            case RenderCommandType::STATE:
                handle_state(cmd);
                break;
            case RenderCommandType::RENDER:
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
    /* transition to */
    transition_render_pass(
        this->render_pass.get_or_null(
            this->swap_chain.render_targets[swap_chain.next_index]),
        false);

    vkCmdEndRenderPass(frame.render_cmd_buffer);
    handle_frame_end();
    vkEndCommandBuffer(frame.render_cmd_buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {frame.image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkCommandBuffer commandBuffers[] = {
        frame.render_cmd_buffer,
    };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = commandBuffers;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &swap_chain.semaphore[swap_chain.next_index];

    if (vkQueueSubmit(graphics_queue, 1, &submitInfo, frame.in_flight_fence) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
    should_present = true;

    handle_destroy();
}

void RenderBackendVK::swap_buffer() {
    if (!should_present) return;

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &swap_chain.semaphore[swap_chain.next_index];

    VkSwapchainKHR swapChains[] = {swap_chain.chain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &swap_chain.next_index;
    presentInfo.pResults = nullptr;  // Optional

    vkQueuePresentKHR(graphics_queue, &presentInfo);
    should_present = false;
}

void RenderBackendVK::handle_update(RenderCommand &cmd) {
    RenderStreamData *stream_data = static_cast<RenderStreamData *>(cmd.data);
    Handle handle = stream_data->handle;
    /* data is right after header */
    void *data = stream_data->get_buffer();
    switch (stream_data->type) {
        case RenderResourceType::VERTEX: {
            HardwareBufferVk *vertex = this->vertices.get_or_null(handle);
            EXPECT_NOT_NULL_BREAK(vertex);
            if (vertex->frequence != UpdateFrequence::PERDRAW) {
                SPDLOG_ERROR("Vertex {} is not perdraw", handle);
                break;
            }
            stream_buffer(vertex, stream_data->size, 1, data);
            break;
        }
        case RenderResourceType::INDEX: {
            HardwareIndexVk *index = this->indices.get_or_null(handle);
            EXPECT_NOT_NULL_BREAK(index);
            if (index->frequence != UpdateFrequence::PERDRAW) {
                SPDLOG_ERROR("Index {} is not perdraw", handle);
                break;
            }
            stream_buffer(index, stream_data->size, 1, data);
            break;
        }
        case RenderResourceType::CONSTANT: {
            HardwareBufferVk *constant = this->constants.get_or_null(handle);
            EXPECT_NOT_NULL_BREAK(constant);
            if (constant->frequence != UpdateFrequence::PERDRAW) {
                SPDLOG_ERROR("Constant {} is not perdraw", handle);
                break;
            }
            stream_buffer(
                constant, stream_data->size,
                device_properties.limits.minUniformBufferOffsetAlignment, data);
            break;
        }
        case RenderResourceType::STORAGE_BUFFER: {
            HardwareBufferVk *buffer = this->ssbos.get_or_null(handle);
            EXPECT_NOT_NULL_RET(buffer);
            if (buffer->frequence != UpdateFrequence::PERDRAW) {
                SPDLOG_ERROR("Storage buffer {} is not perdraw ", handle);
                break;
            }
            stream_buffer(
                buffer, stream_data->size,
                device_properties.limits.minStorageBufferOffsetAlignment, data);
            break;
        }
        default:
            break;
    }
}

void RenderBackendVK::handle_state(RenderCommand &cmd) {
    RenderStateData *state_data = static_cast<RenderStateData *>(cmd.data);
    RenderStateData::Operation *head =
        (RenderStateData::Operation *)(((u64)state_data) +
                                       sizeof(RenderStateData));
    bool viewport_set = false;
    bool scissor_set = false;
    VkViewport viewport_rect{};
    VkRect2D scissor_rect{};
    std::vector<VkClearAttachment> attachments;
    HardwareRenderPassVk *target_rp = nullptr;
    Handle target_rt_handle;
    std::vector<Binding> bindings;
    Frame &frame = frames[get_current_frame_index()];
    bool change_clear = false;
    StateClearFlag clear_flag = 0;
    bool same_target = false;

    for (i32 i = 0; i < state_data->operation_cnt; i++) {
        auto *op = &head[i];
        auto type = op->type;
        switch (type) {
            case RenderStateData::OpType::CLEAR: {
                change_clear = true;
                clear_flag = op->clear_flag;
                break;
            }
            case RenderStateData::OpType::VIEWPORT: {
                viewport_rect.x = op->viewports.view_rects[0].x;
                viewport_rect.y = op->viewports.view_rects[0].y;
                viewport_rect.maxDepth = 1;
                viewport_rect.minDepth = 0;
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
                if (current_render_target == op->render_pass_handle) {
                    same_target = true;
                    break;
                }
                if (op->render_pass_handle == -1) {
                    target_rp = this->render_pass.get_or_null(
                        this->swap_chain.render_targets[swap_chain.next_index]);
                } else {
                    target_rp =
                        this->render_pass.get_or_null(op->render_pass_handle);
                }
                EXPECT_NOT_NULL_BREAK(target_rp);

                target_rt_handle = op->render_pass_handle;
                break;
            }
            case RenderStateData::OpType::BIND_CONSTANT: {
                HardwareBufferVk *constant =
                    this->constants.get_or_null(op->constant.handle);
                EXPECT_NOT_NULL_BREAK(constant);
                i32 id = this->constants.get_id(op->constant.handle);

                bindings.push_back(Binding{.type = RenderResourceType::CONSTANT,
                                           .handle = op->constant.handle,
                                           .resource_id = id,
                                           .binding_point = op->constant.base});
                break;
            }
            case RenderStateData::OpType::BIND_STORAGE_BUFFER: {
                HardwareBufferVk *ssbo =
                    this->ssbos.get_or_null(op->ssbo.handle);
                EXPECT_NOT_NULL_BREAK(ssbo);
                i32 id = this->ssbos.get_id(op->ssbo.handle);
                bindings.push_back(
                    Binding{.type = RenderResourceType::STORAGE_BUFFER,
                            .handle = op->ssbo.handle,
                            .resource_id = id,
                            .binding_point = op->constant.base});
                break;
            }
            default:
                break;
        }
    }
    if (bindings.size() > 0) {
        this->global_bindings = bindings;
    }

    if (target_rp) {
        /* end old render pass */
        vkCmdEndRenderPass(frame.render_cmd_buffer);
        transition_render_pass(get_current_render_pass(), false);
        current_render_target = target_rt_handle;
        transition_render_pass(get_current_render_pass(), true);
        if (change_clear && target_rp->clear_flag != clear_flag) {
            target_rp->clear_flag = clear_flag;
            target_rp->dirty = true;
        }
        create_render_pass(target_rp);
        create_framebuffer(target_rp);
        /* begin new render pass*/
        std::vector<VkClearValue> clears = get_clear_values(target_rp);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = target_rp->render_pass_cache;
        renderPassInfo.framebuffer = target_rp->framebuffer_cache;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.clearValueCount = clears.size();
        renderPassInfo.pClearValues = clears.data();
        renderPassInfo.renderArea.extent =
            VkExtent2D{.width = target_rp->w, .height = target_rp->h};

        vkCmdBeginRenderPass(frame.render_cmd_buffer, &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
    }
    HardwareRenderPassVk *rt = get_current_render_pass();

    VkClearRect clear_rect{};
    clear_rect.rect.extent.width = rt->w;
    clear_rect.rect.extent.height = rt->h;
    clear_rect.baseArrayLayer = 0;
    clear_rect.layerCount = 1;
    if (scissor_set) {
        clear_rect.rect = scissor_rect;
        vkCmdSetScissor(frame.render_cmd_buffer, 0, 1, &scissor_rect);
    }
    if (viewport_set && viewport_rect.width > 0 && viewport_rect.height > 0) {
        if (current_render_target == -1) {
            viewport_rect.y = rt->h - viewport_rect.y;
            viewport_rect.height = -viewport_rect.height;
        }
        vkCmdSetViewport(frame.render_cmd_buffer, 0, 1, &viewport_rect);
    }

    /* only when not changing render pass, we use clear command */
    /* if same render target, then it shouldn't clear again */
    if (!same_target && clear_flag != 0 && target_rp == nullptr) {
        if (clear_flag & CLEAR_COLOR) {
            for (u32 i = 0; i < rt->color_attachments.size(); i++) {
                attachments.emplace_back(VkClearAttachment{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .colorAttachment = i,
                    .clearValue = VkClearValue{.color = {0, 0, 0, 1}}});
            }
        }
        if (clear_flag & CLEAR_DEPTH) {
            attachments.emplace_back(VkClearAttachment{
                .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
                .clearValue = VkClearValue{.depthStencil = {.depth = 1}}});
        }
        if (clear_flag & CLEAR_STENCIL) {
            attachments.emplace_back(VkClearAttachment{
                .aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
                .clearValue = VkClearValue{.depthStencil = {.stencil = 1}}});
        }
        if (clear_rect.rect.extent.width > 0 &&
            clear_rect.rect.extent.height > 0) {
            vkCmdClearAttachments(frame.render_cmd_buffer, attachments.size(),
                                  attachments.data(), 1, &clear_rect);
        }
    }
}

void RenderBackendVK::handle_render(RenderCommand &cmd) {
    Frame &frame = frames[get_current_frame_index()];
    RenderDrawData *draw_data = static_cast<RenderDrawData *>(cmd.data);
    RenderDrawData::Operation *head =
        (RenderDrawData::Operation *)(((u64)draw_data) +
                                      sizeof(RenderDrawData));
    u32 index_type = 0;
    std::vector<VkBuffer> vertex;
    /* used for streaming */
    std::vector<VkDeviceSize> vertex_offsets;
    HardwareIndexVk *index = nullptr;

    std::vector<VertexLayout *> layouts;
    std::vector<Binding> local_bindings;
    HardwarePipelineVk *pipeline =
        this->pipelines.get_or_null(draw_data->pipeline);
    EXPECT_NOT_NULL_RET(pipeline);
    HardwareShaderVk *shader = this->shaders.get_or_null(pipeline->shader);
    u32 push_constant_offset = 0;
    for (i32 i = 0; i < draw_data->operation_cnt; i++) {
        auto *op = &head[i];
        auto type = op->type;
        switch (type) {
            case RenderDrawData::OpType::BIND_VERTEX: {
                HardwareBufferVk *_vertex =
                    this->vertices.get_or_null(op->vertex_handle);
                vertex.push_back(_vertex->buffer);
                vertex_offsets.push_back(_vertex->current);
                break;
            }
            case RenderDrawData::OpType::BIND_INDEX: {
                index = this->indices.get_or_null(op->index_handle);
                EXPECT_NOT_NULL_BREAK(index);
                break;
            }
            case RenderDrawData::OpType::BIND_DESC:
                layouts.push_back(op->vertex_desc);
                break;
            case RenderDrawData::OpType::BIND_TEXTURE: {
                HardwareTextureVk *tex =
                    this->textures.get_or_null(op->texture.texture_handle);
                i32 id = this->textures.get_id(op->texture.texture_handle);
                EXPECT_NOT_NULL_BREAK(tex);
                local_bindings.push_back(
                    Binding{.type = RenderResourceType::TEXTURE,
                            .handle = op->texture.texture_handle,
                            .resource_id = id,
                            .binding_point = op->texture.unit});
                break;
            }
            case RenderDrawData::OpType::BIND_CONSTANT: {
                HardwareBufferVk *constant =
                    this->constants.get_or_null(op->constant.constant_handle);
                EXPECT_NOT_NULL_BREAK(constant);
                i32 id = this->constants.get_id(op->constant.constant_handle);
                local_bindings.push_back(
                    Binding{.type = RenderResourceType::CONSTANT,
                            .handle = op->constant.constant_handle,
                            .resource_id = id,
                            .binding_point = op->constant.unit});
                break;
            }
            case RenderDrawData::OpType::VIEWPORT: {
                VkViewport viewport_rect;
                viewport_rect.x = op->view_rect.x;
                viewport_rect.y = op->view_rect.y;
                viewport_rect.maxDepth = 1.0;
                viewport_rect.minDepth = 0.0;
                viewport_rect.width = op->view_rect.w;
                viewport_rect.height = op->view_rect.h;
                if (current_render_target == -1) {
                    viewport_rect.y =
                        get_current_render_pass()->h - viewport_rect.y;
                    viewport_rect.height = -viewport_rect.height;
                }
                if (viewport_rect.width > 0 && viewport_rect.height > 0) {
                    vkCmdSetViewport(frame.render_cmd_buffer, 0, 1,
                                     &viewport_rect);
                }
                break;
            }
            case RenderDrawData::OpType::SCISSOR: {
                VkRect2D scissor_rect;
                scissor_rect.offset.x = op->scissor_rect.x;
                scissor_rect.offset.y = op->scissor_rect.y;
                scissor_rect.extent.width = op->scissor_rect.w;
                scissor_rect.extent.height = op->scissor_rect.h;
                vkCmdSetScissor(frame.render_cmd_buffer, 0, 1, &scissor_rect);
                break;
            }
            case RenderDrawData::OpType::PUSH_CONSTANT: {
                vkCmdPushConstants(frame.render_cmd_buffer, shader->layout,
                                   VK_SHADER_STAGE_ALL_GRAPHICS,
                                   push_constant_offset, op->push_constant.size,
                                   op->push_constant.data);
                push_constant_offset += op->push_constant.size;
                break;
            }
            default:
                break;
        }
    }
    HardwareRenderPassVk *rt = get_current_render_pass();

    bind_descriptor_set(shader, 0, global_bindings, true);
    if (shader->set_layouts.size() > 1) {
        bind_descriptor_set(shader, 1, local_bindings, false);
    }
    VkPrimitiveTopology primitive = VulkanHelper::primitive(draw_data->type);

    VkPipeline vk_pipeline =
        get_vk_pipeline(pipeline, rt, layouts, primitive, rt->sample_count,
                        draw_data->draw_depth_only, draw_data->depth_clamp);

    vkCmdBindPipeline(frame.render_cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vk_pipeline);
    vkCmdSetDepthWriteEnable(frame.render_cmd_buffer, draw_data->depth_write);
    if (draw_data->depth_test_op == CompareOP::NEVER) {
        vkCmdSetDepthTestEnable(frame.render_cmd_buffer, false);
    } else {
        vkCmdSetDepthTestEnable(frame.render_cmd_buffer, true);
        vkCmdSetDepthCompareOp(frame.render_cmd_buffer,
                               VulkanHelper::compare(draw_data->depth_test_op));
    }

    vkCmdBindVertexBuffers(frame.render_cmd_buffer, 0, vertex.size(),
                           vertex.data(), vertex_offsets.data());
    vkCmdSetPrimitiveTopology(frame.render_cmd_buffer, primitive);
    draw_data->instance_cnt =
        draw_data->instance_cnt < 1 ? 1 : draw_data->instance_cnt;
    if (index) {
        vkCmdBindIndexBuffer(frame.render_cmd_buffer, index->buffer,
                             index->current,
                             VulkanHelper::index_type(index->type));
        vkCmdDrawIndexed(frame.render_cmd_buffer, draw_data->vertex_cnt,
                         draw_data->instance_cnt, draw_data->index_offset,
                         draw_data->vertex_offset, draw_data->instance_offset);
    } else {
        vkCmdDraw(frame.render_cmd_buffer, draw_data->vertex_cnt,
                  draw_data->instance_cnt, draw_data->vertex_offset,
                  draw_data->instance_offset);
    }
}

}  // namespace Seed