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
#include "core/container/ring_buffer.h"

namespace Seed {

struct HardwareBufferVk {
        VkBufferUsageFlags usage;
        VkBuffer buffer;
        VmaAllocation memory;
        UpdateFrequence frequence;
        u32 next_offset = 0;
        u32 last_offset = 0;
        u64 size;
        void *mapped_ptr = nullptr;
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
        std::vector<VkImageLayout> layouts;
};

struct HardwareShaderVk {
        std::string vertex_src;
        std::string geo_src;
        std::string tess_ctrl_src;
        std::string tess_eval_src;
        std::string fragment_src;
        std::vector<VkDescriptorSetLayout> set_layouts;
        VkPipelineLayout layout;
};

struct HardwarePipelineVk {
        ShaderHandle shader;
        RenderRasterizerState rst_state;
        RenderDepthStencilState depth_state;
        RenderBlendState blend_attachment;
};

struct HardwareDepthStencilAttachmentVk {
        bool is_depth;
        bool is_stencil;
        VkFormat image_format;
        TextureHandle texture_handle = NULL_HANDLE;
};

struct HardwareColorAttachmentVk {
        u8 slot;
        VkFormat image_format;
        TextureHandle texture_handle = NULL_HANDLE;
};

struct HardwareRenderTargetVk {
        std::vector<HardwareColorAttachmentVk> color_attachments;
        HardwareDepthStencilAttachmentVk depth_attachment;
        bool depth_only;
        bool dirty = true;
        bool texture_changed = true;
        bool is_swapchain = false;
        u32 w, h;
        VkRenderPass render_pass_cache = nullptr;
        VkFramebuffer framebuffer_cache = nullptr;
};

class RenderBackendVK : public RenderBackend {
    private:
        VkInstance instance;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties device_properties;
        VkDevice device = VK_NULL_HANDLE;
        u32 queue_family_indice;
        VkQueue graphics_queue;
        VkSurfaceKHR surface;
        VkCommandPool command_pool;
        VkCommandBuffer render_cmd_buffer;
        VmaAllocator buffer_allocator;
        VkDescriptorPool descriptor_pool;
        Handle current_render_target;
        VkSemaphore image_available_semaphore;
        VkFence in_flight_fence;
        struct SwapChain {
                VkSwapchainKHR chain;
                VkFormat format;
                std::vector<Handle> textures;
                std::vector<Handle> render_targets;
                std::vector<VkSemaphore> semaphore;
                u32 next_index = 0;
        } swap_chain;

        HandleOwner<HardwareBufferVk> vertices;
        HandleOwner<HardwareIndexVk> indices;
        HandleOwner<HardwareBufferVk> constants;
        HandleOwner<HardwareBufferVk> ssbos;
        HandleIdOwner<HardwareTextureVk> textures;
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
                                VkPipelineLayout layout;
                        } shader;
                        struct {
                                VkRenderPass render_pass;
                                VkFramebuffer framebuffer;
                        } render_target;
                };
        };

        struct StaticBufferUpdate {
                VkBuffer staging_buffer;
                VmaAllocation staging_allocation;
                VkBuffer target_buffer;
                u64 offset;
                u64 size;
        };

        struct DynamicBufferUpdate {
                void *data;
                void *target_buffer;
                VmaAllocation allocation;
                u64 offset;
                u64 size;
        };

        struct ImageUpdate {
                VkBuffer staging_buffer;
                VmaAllocation staging_allocation;
                TextureHandle texture;
                u32 face;
                u32 offx, offy;
                u32 w, h;
        };

        struct Binding {
                RenderResourceType type;
                Handle handle;
                i32 resource_id;
                u32 binding_point;
        };

        /* delay destroy to end of frame */
        std::queue<DestroyResource> destroy_queue;
        std::queue<StaticBufferUpdate> static_buffer_update_queue;
        std::queue<DynamicBufferUpdate> dynamic_buffer_update_queue;

        std::vector<ImageUpdate> image_copy_queue;
        std::vector<HardwareBufferVk *> streams_to_reset;

        std::unordered_map<u64, VkPipeline> pipeline_cache;
        std::unordered_map<u64, VkDescriptorSetLayout> descriptor_layout_cache;
        std::unordered_map<u64, VkDescriptorSet> descriptor_set_cache;

        /* we'll assume global binding */
        /* won't be destroyed at all */
        std::vector<Binding> global_bindings;

/* vulkan setup */
#ifdef VULKAN_DEBUG
        bool enable_validation = true;
#else
        bool enable_validation = false;
#endif
        /* vulkan initiliazation */
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
        void create_descriptor_pool();
        void create_sync_objects();

        /* helper functions */
        void create_staging_buffer(VkBuffer *buffer, VmaAllocation *allocation,
                                   u64 size);
        void create_host_visible_buffer(VkBuffer *buffer,
                                        VmaAllocation *allocation,
                                        VkBufferUsageFlags usage, u64 size,
                                        const void *data);
        void create_gpu_only_buffer(VkBuffer *buffer, VmaAllocation *allocation,
                                    VkBufferUsageFlags usage, u64 size,
                                    const void *data);
        VkImageMemoryBarrier create_image_barrier(HardwareTextureVk *texture,
                                                  VkImageLayout target_layout,
                                                  u32 layer);

        /* create on flight */

        void handle_frame_update();
        void handle_destroy();
        void transition_render_target(HardwareRenderTargetVk *rt,
                                      bool to_attachment);
        VkPipeline create_vk_pipeline(HardwarePipelineVk *pipeline,
                                      HardwareRenderTargetVk *render_target,
                                      std::vector<VertexLayout *> &layouts,
                                      VkPrimitiveTopology primitive);
        VkPipeline get_vk_pipeline(HardwarePipelineVk *pipeline,
                                   HardwareRenderTargetVk *render_target,
                                   std::vector<VertexLayout *> &layouts,
                                   VkPrimitiveTopology primitive);
        void create_render_pass(HardwareRenderTargetVk *render_target,
                                bool is_swapchain);
        void create_framebuffer(HardwareRenderTargetVk *render_target);
        HardwareRenderTargetVk *get_current_render_target();

        VkShaderModule create_shader_module(const std::string &shader);
        bool pick_queue_family(VkPhysicalDevice device);
        void push_buffer_update(HardwareBufferVk *buffer, u64 offset, u64 size,
                                void *data);
        void push_image_update(TextureHandle texture, u32 layer, u32 offx,
                               u32 offy, u32 w, u32 h, void *data);
        void reallocate_buffer(HardwareBufferVk *buffer, u64 size);
        void stream_buffer(HardwareBufferVk *buffer, u64 size, u64 alignment,
                           void *data);
        VkDescriptorSet get_descriptor_set(VkDescriptorSetLayout layout,
                                           std::vector<Binding> &bindings);
        void bind_descriptor_set(HardwareShaderVk *shader, u32 binding,
                                 std::vector<Binding> &bindings);
        /* drawing commands */
        void handle_update(RenderCommand &cmd);
        void handle_state(RenderCommand &cmd);
        void handle_render(RenderCommand &cmd);

    public:
        RenderBackendVK(Window *window);
        ~RenderBackendVK();
        inline RenderBackendType get_type() override {
            return RenderBackendType::VULKAN;
        }
        /* we defer the allocation to allow multithreading. */
        TextureHandle alloc_texture(TextureType type, u32 w, u32 h,
                                    PixelFormat format,
                                    const SamplerProperty &property,
                                    const void *data) override;
        VertexHandle alloc_vertex(u32 stride, u32 element_cnt,
                                  UpdateFrequence frequence,
                                  const void *data) override;
        IndexHandle alloc_indices(IndexType type, u32 element_cnt,
                                  UpdateFrequence frequence,
                                  const void *data) override;
        ConstantHandle alloc_constant(u32 size, const void *data,
                                      UpdateFrequence frequence) override;
        SSBOHandle alloc_storage_buffer(u32 size, const void *data,
                                        UpdateFrequence frequence) override;
        ShaderHandle alloc_shader(const std::string &vertex_code,
                                  const std::string &fragment_code,
                                  const std::string &geometry_code,
                                  const std::string &tess_ctrl_code,
                                  const std::string &tess_eval_code) override;
        void setup_shader_layout(ShaderHandle handle,
                                 const ShaderLayout &layout) override;

        PipelineHandle alloc_pipeline(
            ShaderHandle shader, const RenderRasterizerState &rst_state,
            const RenderDepthStencilState &depth_state,
            const RenderBlendState &blend_state) override;

        RenderTargetHandle alloc_render_target(bool depth_only) override;

        /* We'll use different method for updating different type of buffer */
        /* for STATIC we create a staging buffer to transfer */
        /* for PERFRAME we just map data */
        /* for PERDRAW we use linear allocation with buffer mapping */
        void update(VertexHandle handle, u32 offset, u32 size,
                    void *data) override;
        void update(IndexHandle handle, u32 offset, u32 size,
                    void *data) override;
        void update(ConstantHandle handle, u32 offset, u32 size,
                    void *data) override;
        void update(SSBOHandle handle, u32 offset, u32 size,
                    void *data) override;
        void update(TextureHandle handle, u32 layer, u32 offx, u32 offy, u32 w,
                    u32 h, void *data) override;

        void bind_depth_attachment(RenderTargetHandle handle,
                                   TextureHandle texture, u32 face) override;
        void bind_color_attachment(RenderTargetHandle handle, u8 slot,
                                   TextureHandle texture, u32 face) override;
        void dealloc(TextureHandle handle) override;
        void dealloc(VertexHandle handle) override;
        void dealloc(IndexHandle handle) override;
        void dealloc(ShaderHandle handle) override;
        void dealloc(ConstantHandle handle) override;
        void dealloc(PipelineHandle handle) override;
        void dealloc(SSBOHandle handle) override;
        void dealloc(RenderTargetHandle handle) override;
        void process_commands(std::deque<RenderCommand> &cmd_queue) override;
        void swap_buffer() override;
};

}  // namespace Seed

#endif