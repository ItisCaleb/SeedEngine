#ifndef _SEED_VULKAN_BACKEND_H_
#define _SEED_VULKAN_BACKEND_H_
#include "render_backend.h"
#include "core/container/freelist.h"
#include "core/handle.h"
#include <map>
#include "core/rendering/render_common.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include <vma/vk_mem_alloc.h>

namespace Seed {

struct HardwareBufferVk {
        VkBuffer buffer;
        VmaAllocation memory;
        UpdateFrequence frequence;
        u64 size;
};

struct HardwareIndexVk : public HardwareBufferVk {
        IndexType type;
};

struct HardwareTextureVk {
        u32 w, h;
        TextureType type;
        PixelFormat format;
        VkImage image;
        VkImageView view;
        VkSampler sampler;
        VmaAllocation memory;
};

struct HardwareShaderVk {
        std::string vertex_src;
        std::string geo_src;
        std::string tess_ctrl_src;
        std::string tess_eval_src;
        std::string fragment_src;
        VkDescriptorSetLayout set_layout;
        VkPipelineLayout layout;
};

struct HardwarePipelineVk {
        RenderResource shader;
        RenderRasterizerState rst_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_attachment;
};

struct HardwareAttachmentVk {
        u8 slot;
        bool is_depth;
        VkFormat image_format;
        Handle texture_handle;
};

struct HardwareRenderTargetVk {
        std::vector<HardwareAttachmentVk> attachments;
        bool depth_only;
        bool dirty = true;
        bool texture_changed = true;
        u32 w, h;
        VkRenderPass render_pass_cache = nullptr;
        VkFramebuffer framebuffer_cache = nullptr;
};

class RenderBackendVK : public RenderBackend {
    private:
        RenderResource push_constant;
        VkInstance instance;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties device_properties;
        VkDevice device = VK_NULL_HANDLE;
        u32 queue_family_indice;
        VkQueue graphics_queue;
        VkSurfaceKHR surface;
        VkCommandPool command_pool;
        VkCommandBuffer command_buffer;
        VmaAllocator buffer_allocator;
        HardwareRenderTargetVk *current_render_target;
        struct SwapChain {
                VkSwapchainKHR chain;
                VkFormat format;
                std::vector<Handle> textures;
                std::vector<Handle> render_targets;
        } swap_chain;

        HandleOwner<HardwareBufferVk> vertices;
        HandleOwner<HardwareIndexVk> indices;
        HandleOwner<HardwareBufferVk> constants;
        HandleOwner<HardwareBufferVk> ssbos;
        HandleOwner<HardwareTextureVk> textures;
        HandleOwner<HardwareShaderVk> shaders;
        HandleOwner<HardwarePipelineVk> pipelines;
        HandleOwner<HardwareRenderTargetVk> render_targets;

        struct DestroyResource {
                RenderResourceType type;
                union {
                        struct {
                                VkBuffer buffer;
                                VmaAllocation memory;
                        } buffer;
                        struct {
                                VkImage image;
                                VkImageView view;
                                VkSampler sampler;
                                VmaAllocation memory;
                        } texture;
                        struct {
                                VkDescriptorSetLayout set_layout;
                                VkPipelineLayout layout;
                        } shader;
                        struct {
                                VkRenderPass render_pass;
                                VkFramebuffer framebuffer;
                        } render_target;
                };
        };

        struct BufferCopy {
                VkBuffer staging_buffer;
                VkBuffer target_buffer;
                u64 size;
        };

        struct ImageCopy {
                VkBuffer staging_buffer;
                VkImage target_image;
                u32 w, h;
        };

        /* delay destroy to end of frame */
        std::queue<DestroyResource> destroy_queue;
        std::queue<BufferCopy> buffer_copy_queue;
        std::queue<ImageCopy> image_copy_queue;

        std::unordered_map<u64, VkPipeline> pipeline_cache;

/* vulkan setup */
#ifdef VULKAN_DEBUG
        bool enable_validation = true;
#else
        bool enable_validation = false;
#endif
        void create_instance();
        bool check_validation_support();
        void create_debug_messenger();
        bool pick_physical_device();
        void create_logical_device();
        void create_surface(Window *window);
        void create_swapchain(Window *window);
        void create_image_views();
        void create_swapchain_framebuffer();
        void create_command_pool();
        void create_command_buffer();
        void create_staging_buffer(VkBuffer *buffer, VmaAllocation *allocation,
                                   u64 size);
        void create_host_visible_buffer(VkBuffer *buffer,
                                        VmaAllocation *allocation,
                                        VkBufferUsageFlagBits usage, u64 size,
                                        const void *data);
        void create_gpu_only_buffer(VkBuffer *buffer, VmaAllocation *allocation,
                                    VkBufferUsageFlagBits usage, u64 size,
                                    const void *data);
        VkPipeline create_vk_pipeline(HardwarePipelineVk *pipeline,
                                      HardwareRenderTargetVk *render_target,
                                      std::vector<VertexLayout *> &layouts);
        VkPipeline get_vk_pipeline(HardwarePipelineVk *pipeline,
                                   HardwareRenderTargetVk *render_target,
                                   std::vector<VertexLayout *> &layouts);
        void create_render_pass(HardwareRenderTargetVk *render_target,
                                bool is_swapchain);
        void create_framebuffer(HardwareRenderTargetVk *render_target);

        VkShaderModule create_shader_module(const std::string &shader);
        bool pick_queue_family(VkPhysicalDevice device);

        void reallocate_buffer(HardwareBufferVk *buffer,
                               VkBufferUsageFlagBits usage, u64 size);

        /* drawing commands */
        /* for multithreading purpose */
        void handle_update(RenderCommand &cmd);
        void handle_state(RenderCommand &cmd);
        void handle_render(RenderCommand &cmd);

        /* binding operations */
        void use_texture(u32 unit, RenderResource &rc);

    public:
        RenderBackendVK(Window *window);
        ~RenderBackendVK();
        inline RenderBackendType get_type() override {
            return RenderBackendType::VULKAN;
        }
        /* we defer the allocation to allow multithreading. */
        void alloc_texture(RenderResource *rc, TextureType type, u32 w, u32 h,
                           PixelFormat format, const SamplerProperty &property,
                           const void *data) override;
        void alloc_vertex(RenderResource *rc, u32 stride, u32 element_cnt,
                          UpdateFrequence frequence, const void *data) override;
        void alloc_indices(RenderResource *rc, IndexType type, u32 element_cnt,
                           UpdateFrequence frequence,
                           const void *data) override;
        void alloc_shader(RenderResource *rc, const std::string &vertex_code,
                          const std::string &fragment_code,
                          const std::string &geometry_code,
                          const std::string &tess_ctrl_code,
                          const std::string &tess_eval_code) override;
        void setup_shader_layout(RenderResource *rc,
                                 const ShaderLayout &layout) override;
        void alloc_constant(RenderResource *rc, u32 size,
                            const void *data) override;
        void alloc_pipeline(RenderResource *rc, RenderResource shader,
                            const RenderRasterizerState &rst_state,
                            const RenderDepthStencilState &depth_state,
                            const RenderBlendState &blend_state) override;

        void alloc_render_target(RenderResource *rc, bool depth_only) override;
        void alloc_buffer(RenderResource *rc, u32 size,
                          const void *data) override;
        void dealloc(RenderResource *rc) override;
        void process_commands(std::deque<RenderCommand> &cmd_queue) override;
        void swap_buffer() override;
};

}  // namespace Seed

#endif