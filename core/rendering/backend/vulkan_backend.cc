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
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
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

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &features12;
    deviceFeatures2.features.tessellationShader = true;
    deviceFeatures2.features.samplerAnisotropy = true;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pNext = &deviceFeatures2;
    createInfo.pEnabledFeatures = nullptr;
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
        rt.is_swapchain = true;
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
            rt.dirty = false;
        }
        create_framebuffer(&rt);
        Handle handle = this->render_targets.insert(rt);
        this->swap_chain.render_targets.push_back(handle);
    }
    current_render_target = this->swap_chain.render_targets[0];
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

void RenderBackendVK::create_descriptor_pool() {
    VkDescriptorPoolSize ssbos{};
    VkDescriptorPoolSize ubos{};
    VkDescriptorPoolSize samplers{};
    ssbos.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    ssbos.descriptorCount = 16;
    ubos.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubos.descriptorCount = 50;
    samplers.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplers.descriptorCount = 500;
    std::vector<VkDescriptorPoolSize> size = {ssbos, ubos, samplers};

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

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &image_available_semaphore) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &in_flight_fence) !=
            VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores!");
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
                        allocation, nullptr) != VK_TRUE) {
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
                                  bufferInfo.size);
        this->buffer_copy_queue.push(BufferCopy{.staging_buffer = stagingBuffer,
                                                .target_buffer = *buffer,
                                                .size = size});
    }
}

inline VkImageMemoryBarrier RenderBackendVK::create_image_barrier(
    HardwareTextureVk *texture, VkImageLayout target_layout, u32 layer) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = texture->layout;
    barrier.newLayout = target_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->image;
    barrier.subresourceRange.aspectMask =
        VulkanHelper::aspect_flag(texture->format);
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = layer;
    barrier.subresourceRange.layerCount = 1;

    texture->layout = target_layout;
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

    this->destroy_queue.push(DestroyResource{
        .type = RenderResourceType::VERTEX,
        .buffer = {.buffer = buffer->buffer, .memory = buffer->memory}});

    /* rebind */
    buffer->size = size;
    buffer->buffer = new_buffer;
    buffer->memory = allocation;
}

VkDescriptorSet RenderBackendVK::get_descriptor_set(
    VkDescriptorSetLayout layout, std::vector<Binding> &bindings) {
    Hash _hash;
    for (auto &binding : bindings) {
        _hash.update(&binding.binding_point);
        _hash.update(&binding.type);
        if (binding.type == RenderResourceType::TEXTURE) {
            _hash.update(&binding.image.view);
            _hash.update(&binding.image.sampler);
        } else {
            _hash.update(&binding.buffer);
        }
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
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet set{};
    if (vkAllocateDescriptorSets(device, &allocInfo, &set) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    std::vector<VkWriteDescriptorSet> writes;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;

    writes.reserve(bindings.size());
    bufferInfos.reserve(bindings.size());
    imageInfos.reserve(bindings.size());

    for (auto &binding : bindings) {
        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = binding.binding_point;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;

        if (binding.type == RenderResourceType::CONSTANT) {
            VkDescriptorBufferInfo info{};
            info.buffer = binding.buffer;
            info.offset = 0;
            info.range = VK_WHOLE_SIZE;

            bufferInfos.push_back(info);

            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.pBufferInfo = &bufferInfos.back();
        } else if (binding.type == RenderResourceType::BUFFER) {
            VkDescriptorBufferInfo info{};
            info.buffer = binding.buffer;
            info.offset = 0;
            info.range = VK_WHOLE_SIZE;

            bufferInfos.push_back(info);

            write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            write.pBufferInfo = &bufferInfos.back();
        } else if (binding.type == RenderResourceType::TEXTURE) {
            VkDescriptorImageInfo info{};
            info.imageView = binding.image.view;
            info.sampler = binding.image.sampler;
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

void RenderBackendVK::alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                    u32 h, PixelFormat format,
                                    const SamplerProperty &property,
                                    const void *data) {
    VkImage image;
    VkImageView image_view;
    VkSampler sampler;
    VkImageCreateInfo imageInfo{};

    bool is_depth = VulkanHelper::is_depth(format);
    bool is_stencil = VulkanHelper::is_stencil(format);
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.usage =
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
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

    VkDescriptorImageInfo imageDescriptor{};
    imageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageDescriptor.imageView = image_view;
    imageDescriptor.sampler = sampler;
    VkImageLayout layout = (data && type != TextureType::TEXTURE_CUBEMAP)
                               ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                               : VK_IMAGE_LAYOUT_UNDEFINED;

    rc->handle = this->textures.insert({.w = w,
                                        .h = h,
                                        .type = type,
                                        .format = format,
                                        .image = image,
                                        .view = image_view,
                                        .sampler = sampler,
                                        .memory = allocation,
                                        .layout = layout});

    /* upload using staging buffer */
    /* we don't updload cubemap here */
    if (data && type != TextureType::TEXTURE_CUBEMAP) {
        size_t size = w * h * get_pixel_format_size(format);
        VkBuffer stagingBuffer;
        VmaAllocation stagingAllocation;
        create_staging_buffer(&stagingBuffer, &stagingAllocation, size);
        vmaCopyMemoryToAllocation(buffer_allocator, data, stagingAllocation, 0,
                                  size);
        this->image_copy_queue.push_back(
            ImageCopy{.staging_buffer = stagingBuffer,
                      .target_image = image,
                      .w = w,
                      .h = h,
                      .format = format});
    }
}

void RenderBackendVK::alloc_vertex(RenderResource *rc, u32 stride,
                                   u32 element_cnt, UpdateFrequence frequence,
                                   const void *data) {
    VkBuffer vertex;
    VmaAllocation allocation;
    /* if no element then just allocate */
    /* a big buffer to minimize reallocation*/
    if (element_cnt == 0) {
        element_cnt = 1000;
    }
    create_gpu_only_buffer(&vertex, &allocation,
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           stride * element_cnt, data);
    // if (frequence == UpdateFrequence::IMMUTABLE) {
    // } else {
    //     create_host_visible_buffer(&vertex, &allocation,
    //                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    //                                stride * element_cnt, data);
    // }

    rc->handle = this->vertices.insert({.buffer = vertex,
                                        .memory = allocation,
                                        .frequence = frequence,
                                        .size = stride * element_cnt});
}

void RenderBackendVK::alloc_indices(RenderResource *rc, IndexType type,
                                    u32 element_cnt, UpdateFrequence frequence,
                                    const void *data) {
    VkBuffer indice;
    VmaAllocation allocation;
    /* if no element then just allocate */
    /* a big buffer to minimize reallocation*/
    if (element_cnt == 0) {
        element_cnt = 1000;
    }
    u64 size = get_index_size(type) * element_cnt;
    create_gpu_only_buffer(&indice, &allocation,
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size, data);
    // if (frequence == UpdateFrequence::IMMUTABLE) {
    // } else {
    //     create_host_visible_buffer(
    //         &indice, &allocation, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size,
    //         data);
    // }

    HardwareIndexVk index;
    index.type = type;
    index.buffer = indice;
    index.size = size;
    index.frequence = frequence;
    index.memory = allocation;
    rc->handle = this->indices.insert(index);
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
                                        .layout = nullptr});
}

void RenderBackendVK::setup_shader_layout(RenderResource *rc,
                                          const ShaderLayout &shader_layout) {
    HardwareShaderVk *shader = this->shaders.get_or_null(rc->handle);
    EXPECT_NOT_NULL_RET(shader);
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    std::vector<VkPushConstantRange> ranges;

    std::vector<VkDescriptorSetLayout> set_layouts;
    for (const ShaderBindingSet &set : shader_layout.get_binding_sets()) {
        VkDescriptorSetLayout set_layout;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        Hash _hash;
        for (const ShaderBinding &binding : set.bindings) {
            VkDescriptorSetLayoutBinding _binding{};
            _binding.binding = binding.binding_point;
            _binding.descriptorCount = binding.count;
            _binding.descriptorType =
                VulkanHelper::descriptor_type(binding.type);
            _binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
            _hash.update(&_binding.binding);
            _hash.update(&_binding.descriptorCount);
            _hash.update(&_binding.descriptorType);

            bindings.push_back(_binding);
        }
        u64 hash = _hash.digest();
        auto iter = descriptor_layout_cache.find(hash);
        if (iter != descriptor_layout_cache.end()) {
            set_layouts.push_back(iter->second);
            continue;
        }
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = bindings.size();
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                        &set_layout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
        descriptor_layout_cache.emplace(hash, set_layout);
        set_layouts.push_back(set_layout);
    }
    /* bindings */

    for (const PushConstantRange &range : shader_layout.get_push_contants()) {
        VkPushConstantRange _range;
        _range.offset = range.offset;
        _range.size = range.size;
        _range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        ranges.push_back(_range);
    }

    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = set_layouts.size();
    pipelineLayoutInfo.pSetLayouts = set_layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
    pipelineLayoutInfo.pPushConstantRanges = ranges.data();
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan pipeline layout!");
    }
    shader->set_layouts = set_layouts;
    shader->layout = pipelineLayout;
}

void RenderBackendVK::alloc_constant(RenderResource *rc, u32 size,
                                     const void *data) {
    VkBuffer constant;
    VmaAllocation allocation;
    create_gpu_only_buffer(&constant, &allocation,
                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, size, data);
    rc->handle = this->constants.insert({.buffer = constant,
                                         .memory = allocation,
                                         .frequence = UpdateFrequence::PERFRAME,
                                         .size = size});
}

void RenderBackendVK::alloc_buffer(RenderResource *rc, u32 size,
                                   const void *data) {
    VkBuffer buffer;
    VmaAllocation allocation;

    create_gpu_only_buffer(&buffer, &allocation,
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size, data);
    rc->handle = this->ssbos.insert({.buffer = buffer,
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
    pipeline.shader = shader;
    pipeline.rst_state = rst_state;
    pipeline.depth_state = depth_state;
    pipeline.blend_attachment = blend_state;
    rc->handle = this->pipelines.insert(pipeline);
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

    // if (render_target->render_pass_cache != nullptr) {
    //     vkDestroyRenderPass(device, render_target->render_pass_cache,
    //     nullptr);
    // }

    for (auto attachment : render_target->attachments) {
        if (attachment.is_depth || attachment.is_stencil) {
            depthAttachment.format = attachment.image_format;
            depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            depthAttachment.loadOp = is_swapchain ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                  : VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.stencilLoadOp = depthAttachment.loadOp;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthAttachment.initialLayout =
                (attachment.is_depth && attachment.is_stencil)
                    ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                : attachment.is_depth
                    ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachment.finalLayout = depthAttachment.initialLayout;

            allAttachments.push_back(depthAttachment);

            depthRef.attachment = i;
            depthRef.layout = depthAttachment.finalLayout;
            subpass.pDepthStencilAttachment = &depthRef;
        } else {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = attachment.image_format;
            colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachment.loadOp = is_swapchain ? VK_ATTACHMENT_LOAD_OP_CLEAR
                                                  : VK_ATTACHMENT_LOAD_OP_LOAD;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.initialLayout =
                is_swapchain ? VK_IMAGE_LAYOUT_UNDEFINED
                             : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
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

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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
    framebufferInfo.attachmentCount = attachments.size();
    framebufferInfo.pAttachments = attachments.data();
    render_target->w = width;
    render_target->h = height;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &render_target->framebuffer_cache) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Vulkan framebuffer!");
    }
}

HardwareRenderTargetVk *RenderBackendVK::get_current_render_target() {
    if (current_render_target == -1) {
        return this->render_targets.get_or_null(
            this->swap_chain.render_targets[swap_chain.next_index]);
    } else {
        return this->render_targets.get_or_null(current_render_target);
    }
}

VkPipeline RenderBackendVK::get_vk_pipeline(
    HardwarePipelineVk *pipeline, HardwareRenderTargetVk *render_target,
    std::vector<VertexLayout *> &layouts, VkPrimitiveTopology primitive) {
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
    _hash.update(&primitive);

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
        vk_pipeline =
            create_vk_pipeline(pipeline, render_target, layouts, primitive);
        pipeline_cache.emplace(hash, vk_pipeline);
    } else {
        vk_pipeline = iter->second;
    }
    return vk_pipeline;
}

void RenderBackendVK::handle_create() {
    while (!buffer_copy_queue.empty()) {
        BufferCopy &copy = buffer_copy_queue.front();
        VkBufferCopy _copy{};
        _copy.srcOffset = 0;
        _copy.dstOffset = 0;
        _copy.size = copy.size;
        vkCmdCopyBuffer(command_buffer, copy.staging_buffer, copy.target_buffer,
                        1, &_copy);
        this->destroy_queue.push(
            DestroyResource{.type = RenderResourceType::VERTEX,
                            .buffer = {.buffer = copy.staging_buffer,
                                       .memory = copy.staging_allocation}});
        buffer_copy_queue.pop();
    }
    std::vector<VkImageMemoryBarrier> transfer_barriers;
    std::vector<VkImageMemoryBarrier> shader_barriers;

    for (ImageCopy &copy : image_copy_queue) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = copy.target_image;
        barrier.subresourceRange.aspectMask =
            VulkanHelper::aspect_flag(copy.format);
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        transfer_barriers.push_back(barrier);
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        shader_barriers.push_back(barrier);
    }

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, transfer_barriers.size(),
                         transfer_barriers.data());

    for (ImageCopy &copy : image_copy_queue) {
        VkBufferImageCopy _copy{};
        _copy.bufferOffset = 0;
        _copy.bufferRowLength = 0;
        _copy.bufferImageHeight = 0;

        _copy.imageSubresource.aspectMask =
            VulkanHelper::aspect_flag(copy.format);
        _copy.imageSubresource.mipLevel = 0;
        _copy.imageSubresource.baseArrayLayer = 0;
        _copy.imageSubresource.layerCount = 1;

        _copy.imageOffset = {0, 0, 0};
        _copy.imageExtent = {copy.w, copy.h, 1};
        vkCmdCopyBufferToImage(command_buffer, copy.staging_buffer,
                               copy.target_image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &_copy);
        this->destroy_queue.push(
            DestroyResource{.type = RenderResourceType::VERTEX,
                            .buffer = {.buffer = copy.staging_buffer,
                                       .memory = copy.staging_allocation}});
    }
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0,
                         nullptr, shader_barriers.size(),
                         shader_barriers.data());
    image_copy_queue.clear();
}

void RenderBackendVK::handle_destroy() {
    while (!destroy_queue.empty()) {
        DestroyResource &destroy = destroy_queue.front();
        switch (destroy.type) {
            case RenderResourceType::VERTEX:
            case RenderResourceType::INDEX:
            case RenderResourceType::CONSTANT:
            case RenderResourceType::BUFFER:
                vmaDestroyBuffer(buffer_allocator, destroy.buffer.buffer,
                                 destroy.buffer.memory);
                break;
            case RenderResourceType::SHADER:
                /* Since layouts might be reused, we won't destroy them. */
                // vkDestroyDescriptorSetLayout(device,
                // destroy.shader.set_layout,
                //                              nullptr);
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

void RenderBackendVK::transition_render_target(HardwareRenderTargetVk *rt,
                                               bool to_attachment) {
    /* we don't need swapchain to transition*/
    if (rt->is_swapchain) {
        return;
    }
    std::vector<VkImageMemoryBarrier> barriers;
    for (HardwareAttachmentVk &attachment : rt->attachments) {
        VkImageLayout target_layout;
        if (to_attachment) {
            target_layout =
                (attachment.is_stencil && attachment.is_depth)
                    ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                : attachment.is_depth ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL
                : attachment.is_stencil
                    ? VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        } else {
            target_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
        barriers.push_back(create_image_barrier(
            this->textures.get_or_null(attachment.texture_handle),
            target_layout, 0));
    }
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                         VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0, nullptr, 0,
                         nullptr, barriers.size(), barriers.data());
}

VkPipeline RenderBackendVK::create_vk_pipeline(
    HardwarePipelineVk *pipeline, HardwareRenderTargetVk *render_target,
    std::vector<VertexLayout *> &layouts, VkPrimitiveTopology primitive) {
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

    VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
    assemblyInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyInfo.topology = primitive;
    assemblyInfo.primitiveRestartEnable = false;

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

    VkPipelineTessellationStateCreateInfo tessellation{};
    tessellation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
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
            this->vertices.remove(rc->handle);
            break;
        }

        case RenderResourceType::INDEX: {
            HardwareIndexVk *index = this->indices.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(index);
            this->destroy_queue.push(DestroyResource{
                .type = RenderResourceType::INDEX,
                .buffer = {.buffer = index->buffer, .memory = index->memory}});
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
            this->textures.remove(rc->handle);
            break;
        }
        case RenderResourceType::SHADER: {
            HardwareShaderVk *shader = this->shaders.get_or_null(rc->handle);
            EXPECT_NOT_NULL_RET(shader);
            this->destroy_queue.push(
                DestroyResource{.type = RenderResourceType::SHADER,
                                .shader = {.layout = shader->layout}});
            this->shaders.remove(rc->handle);
            break;
        }
        case RenderResourceType::PIPELINE: {
            this->pipelines.remove(rc->handle);
            break;
        }
        case RenderResourceType::RENDER_TARGET: {
            HardwareRenderTargetVk *rt = nullptr;
            rt = this->render_targets.get_or_null(rc->handle);
            EXPECT_NOT_NULL_BREAK(rt);
            this->destroy_queue.push(DestroyResource{
                .type = RenderResourceType::VERTEX,
                .render_target = {.render_pass = rt->render_pass_cache,
                                  .framebuffer = rt->framebuffer_cache}});
            this->render_targets.remove(rc->handle);
            break;
        }
        default:
            break;
    }
}

void RenderBackendVK::process_commands(std::deque<RenderCommand> &cmd_queue) {
    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fence);

    vkAcquireNextImageKHR(device, swap_chain.chain, UINT64_MAX,
                          image_available_semaphore, VK_NULL_HANDLE,
                          &swap_chain.next_index);
    vkResetCommandBuffer(command_buffer, 0);
    handle_destroy();

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                   // Optional
    beginInfo.pInheritanceInfo = nullptr;  // Optional

    if (vkBeginCommandBuffer(command_buffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    handle_create();

    RenderCommandType last_type;
    bool render_pass_begin = false;
    const VkClearValue clears[] = {VkClearValue{.color = {0, 0, 0, 1}}};
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = clears;

    while (!cmd_queue.empty()) {
        RenderCommand &cmd = cmd_queue.front();
        switch (cmd.type) {
            case RenderCommandType::UPDATE:
                if (render_pass_begin) {
                    vkCmdEndRenderPass(command_buffer);
                    render_pass_begin = false;
                }
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
                    HardwareRenderTargetVk *rt = get_current_render_target();
                    create_render_pass(rt, false);
                    create_framebuffer(rt);
                    renderPassInfo.renderPass = rt->render_pass_cache;
                    renderPassInfo.framebuffer = rt->framebuffer_cache;
                    renderPassInfo.renderArea.extent =
                        VkExtent2D{.width = rt->w, .height = rt->h};

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

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {image_available_semaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &swap_chain.semaphore[swap_chain.next_index];

    if (vkQueueSubmit(graphics_queue, 1, &submitInfo, in_flight_fence) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
}

void RenderBackendVK::swap_buffer() {
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
}

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
                SPDLOG_ERROR("Trying to update immutable vertex {}", rc.handle);
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
            u64 size =
                _texture.w * _texture.h * get_pixel_format_size(tex->format);
            create_staging_buffer(&staging_buffer, &staging_allocation, size);
            vmaCopyMemoryToAllocation(buffer_allocator, data,
                                      staging_allocation, 0, size);
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;

            region.imageSubresource.aspectMask =
                VulkanHelper::aspect_flag(tex->format);
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = update_data->texture.face;
            region.imageSubresource.layerCount = 1;

            region.imageOffset = {(i32)_texture.x_off, (i32)_texture.y_off, 0};
            region.imageExtent = {_texture.w, _texture.h, 1};
            /* for cubemap specialization */
            if (tex->type == TextureType::TEXTURE_CUBEMAP) {
                tex->layout = VK_IMAGE_LAYOUT_UNDEFINED;
            }
            VkImageMemoryBarrier barrier =
                create_image_barrier(tex, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                     update_data->texture.face);
            vkCmdPipelineBarrier(command_buffer,
                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &barrier);
            vkCmdCopyBufferToImage(command_buffer, staging_buffer, tex->image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &region);
            barrier = create_image_barrier(
                tex, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                update_data->texture.face);
            vkCmdPipelineBarrier(command_buffer,
                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
                                 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 0,
                                 nullptr, 0, nullptr, 1, &barrier);
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
                HardwareAttachmentVk &attachment = rt->attachments[i];
                if (attachment.is_depth && update_data->attachment.is_depth ||
                    attachment.slot && update_data->attachment.slot) {
                    if (attachment.image_format != format) {
                        attachment.image_format = format;
                        rt->dirty = true;
                    }
                    attachment.texture_handle =
                        update_data->attachment.texture.handle;
                    rt->texture_changed = true;

                    flag = true;
                }
            }

            /* not found so we insert*/
            if (!flag) {
                rt->attachments.push_back(HardwareAttachmentVk{
                    .slot = update_data->attachment.slot,
                    .is_depth = update_data->attachment.is_depth,
                    .is_stencil = VulkanHelper::is_stencil(tex->format),
                    .image_format = format,
                    .texture_handle = update_data->attachment.texture.handle});
                rt->dirty = true;
                rt->texture_changed = true;
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
    VkViewport viewport_rect{};
    VkRect2D scissor_rect{};
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
                            VkClearValue{.depthStencil = {.depth = 1}}});
                }
                if (op->clear_flag & CLEAR_STENCIL) {
                    attachments.emplace_back(VkClearAttachment{
                        .aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
                        .clearValue =
                            VkClearValue{.depthStencil = {.stencil = 1}}});
                }
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
                HardwareRenderTargetVk *target_rt = nullptr;
                if (op->render_target.handle == -1) {
                    target_rt = this->render_targets.get_or_null(
                        this->swap_chain.render_targets[swap_chain.next_index]);
                } else {
                    target_rt = this->render_targets.get_or_null(
                        op->render_target.handle);
                }
                EXPECT_NOT_NULL_BREAK(target_rt);
                transition_render_target(get_current_render_target(), false);

                current_render_target = op->render_target.handle;
                transition_render_target(get_current_render_target(), true);
                break;
            }
            case RenderStateData::OpType::BIND_BUFFERBASE: {
                HardwareBufferVk *buffer = nullptr;
                if (op->bufferbase.buffer.type == RenderResourceType::BUFFER) {
                    buffer =
                        this->ssbos.get_or_null(op->bufferbase.buffer.handle);
                } else if (op->bufferbase.buffer.type ==
                           RenderResourceType::CONSTANT) {
                    buffer = this->constants.get_or_null(
                        op->bufferbase.buffer.handle);
                }
                EXPECT_NOT_NULL_BREAK(buffer);
                this->global_bindings.push_back(
                    Binding{.type = op->bufferbase.buffer.type,
                            .buffer = buffer->buffer,
                            .binding_point = op->bufferbase.base});
                break;
            }
            default:
                break;
        }
    }
    HardwareRenderTargetVk *rt = get_current_render_target();

    clear_rect.rect.extent.width = rt->w;
    clear_rect.rect.extent.height = rt->h;
    clear_rect.baseArrayLayer = 0;
    clear_rect.layerCount = 1;
    if (scissor_set) {
        clear_rect.rect = scissor_rect;
        vkCmdSetScissor(command_buffer, 0, 1, &scissor_rect);
    }
    if (viewport_set) {
        vkCmdSetViewport(command_buffer, 0, 1, &viewport_rect);
    }
    const VkClearValue clears[] = {VkClearValue{.color = {0, 0, 0, 1}}};
    if (!attachments.empty()) {
        create_render_pass(rt, false);
        create_framebuffer(rt);
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = rt->render_pass_cache;
        renderPassInfo.framebuffer = rt->framebuffer_cache;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clears;
        renderPassInfo.renderArea.extent =
            VkExtent2D{.width = rt->w, .height = rt->h};

        vkCmdBeginRenderPass(command_buffer, &renderPassInfo,
                             VK_SUBPASS_CONTENTS_INLINE);
        if (!rt->is_swapchain) {
            vkCmdClearAttachments(command_buffer, attachments.size(),
                                  attachments.data(), 1, &clear_rect);
        }

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

    std::vector<VertexLayout *> layouts;
    std::vector<Binding> texture_bindings;
    HardwarePipelineVk *pipeline =
        this->pipelines.get_or_null(draw_data->pipeline.handle);
    HardwareShaderVk *shader =
        this->shaders.get_or_null(pipeline->shader.handle);
    HardwareIndexVk *index = nullptr;
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
                index = this->indices.get_or_null(op->index_rc.handle);
                EXPECT_NOT_NULL_BREAK(index);
                break;
            }
            case RenderDrawData::OpType::BIND_DESC:
                layouts.push_back(op->vertex_desc);
                break;
            case RenderDrawData::OpType::BIND_TEXTURE: {
                HardwareTextureVk *tex =
                    this->textures.get_or_null(op->texture.rc.handle);
                EXPECT_NOT_NULL_BREAK(tex);
                texture_bindings.push_back(Binding{
                    .type = RenderResourceType::TEXTURE,
                    .image = {.view = tex->view, .sampler = tex->sampler},
                    .binding_point = op->texture.unit});

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
                vkCmdPushConstants(command_buffer, shader->layout,
                                   VK_SHADER_STAGE_ALL_GRAPHICS, 0,
                                   op->constant.size, op->constant.data);

                break;
            }
            default:
                break;
        }
    }
    HardwareRenderTargetVk *rt = get_current_render_target();

    VkDescriptorSet globalSet =
        get_descriptor_set(shader->set_layouts[0], global_bindings);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            shader->layout, 0, 1, &globalSet, 0, nullptr);
    if (shader->set_layouts.size() > 1) {
        VkDescriptorSet matSet =
            get_descriptor_set(shader->set_layouts[1], texture_bindings);
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                shader->layout, 1, 1, &matSet, 0, nullptr);
    }
    VkPrimitiveTopology primitive = VulkanHelper::primitive(draw_data->type);

    VkPipeline vk_pipeline = get_vk_pipeline(pipeline, rt, layouts, primitive);

    VkDeviceSize offsets[] = {0};
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vk_pipeline);

    vkCmdBindVertexBuffers(command_buffer, 0, vertex.size(), vertex.data(),
                           offsets);
    vkCmdSetPrimitiveTopology(command_buffer, primitive);
    draw_data->instance_cnt =
        draw_data->instance_cnt < 1 ? 1 : draw_data->instance_cnt;
    if (index) {
        vkCmdBindIndexBuffer(command_buffer, index->buffer, 0,
                             VulkanHelper::index_type(index->type));
        vkCmdDrawIndexed(command_buffer, draw_data->vertex_cnt,
                         draw_data->instance_cnt, draw_data->index_offset,
                         draw_data->vertex_offset, draw_data->instance_offset);
    } else {
        vkCmdDraw(command_buffer, draw_data->vertex_cnt,
                  draw_data->instance_cnt, draw_data->vertex_offset,
                  draw_data->instance_offset);
    }
}

}  // namespace Seed