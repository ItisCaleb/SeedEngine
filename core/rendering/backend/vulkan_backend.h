#ifndef _SEED_VULKAN_BACKEND_H_
#define _SEED_VULKAN_BACKEND_H_
#include "render_backend.h"
#include "core/window.h"
#include "core/handle.h"
#include "core/rendering/render_common.h"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#ifdef __APPLE__
#include <vk_mem_alloc.h>
#else
#include <vma/vk_mem_alloc.h>
#endif
#include "core/container/ring_buffer.h"
#include <list>

namespace Seed {

struct HardwareBufferVk {
        VkBufferUsageFlags usage;
        VkBuffer buffer;
        VmaAllocation memory;
        UpdateFrequence frequence;
        u32 current = 0;
        u32 head = 0;
        u32 tail = 0;
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
        VkSampleCountFlagBits sample_count;
        VkImage msaa_image;
        VkImageView msaa_view;
        VmaAllocation msaa_memory;
        std::vector<VkImageLayout> layouts;
        void *mapped_ptr = nullptr;
};

struct DescriptorSetLayout {
        VkDescriptorSetLayout vk_layout;
        ShaderBindingSet set;
};

struct HardwareShaderVk {
        std::string vertex_src;
        std::string geo_src;
        std::string tess_ctrl_src;
        std::string tess_eval_src;
        std::string fragment_src;
        std::vector<DescriptorSetLayout *> set_layouts;
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

struct HardwareRenderPassVk {
        std::vector<HardwareColorAttachmentVk> color_attachments;
        HardwareDepthStencilAttachmentVk depth_attachment;
        VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
        bool dirty = true;
        bool texture_changed = true;
        bool is_swapchain = false;
        u32 w, h;
        VkRenderPass render_pass_cache = nullptr;
        VkFramebuffer framebuffer_cache = nullptr;
};

class RenderBackendVK : public RenderBackend {
    protected:
        Window *current_window;
        VkInstance instance;
        VkPhysicalDevice physical_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties device_properties;
        VkDevice device = VK_NULL_HANDLE;
        u32 queue_family_indice;
        VkQueue graphics_queue;
        VkSurfaceKHR surface;
        VkCommandPool command_pool;
        VmaAllocator buffer_allocator;
        VkDescriptorPool descriptor_pool;
        Handle current_render_target;

        struct SwapChain {
                VkSwapchainKHR chain;
                VkFormat format;
                std::vector<Handle> textures;
                std::vector<Handle> render_targets;
                std::vector<VkSemaphore> semaphore;
                u32 next_index = 0;
        } swap_chain;

        struct Frame {
                VkCommandBuffer render_cmd_buffer;
                VkSemaphore image_available_semaphore;
                VkFence in_flight_fence;
                struct StreamBufferUsage {
                        HardwareBufferVk *buffer;
                        u64 size;
                };
                std::vector<StreamBufferUsage> usages;
        } frames[FRAMES_IN_FLIGHT];

        HandleOwner<HardwareBufferVk> vertices;
        HandleOwner<HardwareIndexVk> indices;
        HandleOwner<HardwareBufferVk> constants;
        HandleOwner<HardwareBufferVk> ssbos;
        HandleIdOwner<HardwareTextureVk> textures;
        HandleOwner<HardwareShaderVk> shaders;
        HandleOwner<HardwarePipelineVk> pipelines;
        HandleOwner<HardwareRenderPassVk> render_pass;

        struct DestroyResource {
                RenderResourceType type;
                u8 frame_count = 0;
                bool mapped;
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
        std::list<DestroyResource> destroy_list;
        RingBuffer<StaticBufferUpdate> static_buffer_update_queue;
        RingBuffer<DynamicBufferUpdate> dynamic_buffer_update_queue;

        RingBuffer<ImageUpdate> image_copy_queue;
        RingBuffer<TextureHandle> mappable_image_transition_queue;

        std::unordered_map<u64, VkPipeline> pipeline_cache;
        std::unordered_map<u64, DescriptorSetLayout> descriptor_layout_cache;
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
        void recreate_swapchain(Window *window);
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
        void transition_render_pass(HardwareRenderPassVk *rt,
                                    bool to_attachment);
        VkPipeline create_vk_pipeline(HardwarePipelineVk *pipeline,
                                      HardwareRenderPassVk *render_target,
                                      std::vector<VertexLayout *> &layouts,
                                      VkPrimitiveTopology primitive,
                                      VkSampleCountFlagBits sample_count,
                                      bool draw_depth_only, bool depth_clamp);
        VkPipeline get_vk_pipeline(HardwarePipelineVk *pipeline,
                                   HardwareRenderPassVk *render_target,
                                   std::vector<VertexLayout *> &layouts,
                                   VkPrimitiveTopology primitive,
                                   VkSampleCountFlagBits sample_count,
                                   bool draw_depth_only, bool depth_clamp);
        void create_render_pass(HardwareRenderPassVk *render_target,
                                bool is_swapchain);
        void create_framebuffer(HardwareRenderPassVk *render_target);
        HardwareRenderPassVk *get_current_render_pass();

        VkShaderModule create_shader_module(const std::string &shader);
        bool pick_queue_family(VkPhysicalDevice device);
        void push_buffer_update(HardwareBufferVk *buffer, u64 offset, u64 size,
                                void *data);
        void push_image_update(TextureHandle texture, u32 layer, u32 offx,
                               u32 offy, u32 w, u32 h, void *data);
        void push_buffer_destroy(RenderResourceType type, VkBuffer buffer,
                                 VmaAllocation allocation, bool mapped);
        void reallocate_buffer(HardwareBufferVk *buffer, u64 size);
        void stream_buffer(HardwareBufferVk *buffer, u64 size, u64 alignment,
                           void *data);
        VkDescriptorSet get_descriptor_set(DescriptorSetLayout *layout,
                                           std::vector<Binding> &bindings, bool is_global);
        void bind_descriptor_set(HardwareShaderVk *shader, u32 binding,
                                 std::vector<Binding> &bindings, bool is_global);
        /* drawing commands */
        void handle_update(RenderCommand &cmd);
        void handle_state(RenderCommand &cmd);
        void handle_render(RenderCommand &cmd);
        RenderBackendVK() = default;
    public:
        RenderBackendVK(Window *window);
        ~RenderBackendVK();
        inline RenderBackendType get_type() override {
            return RenderBackendType::VULKAN;
        }
        /* we defer the allocation to allow multithreading. */
        TextureHandle alloc_texture(TextureType type, u32 w, u32 h,
                                    PixelFormat format, MSAAType msaa_type,
                                    const SamplerProperty &property,
                                    const void *data) override;
        TextureHandle alloc_mappable_texture(TextureType type, u32 w, u32 h,
                                             PixelFormat format,
                                             const SamplerProperty &property,
                                             const void *data) override;
        void query_texture_size(TextureHandle handle, u32 *w, u32 *h) override;

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

        RenderPassHandle alloc_render_pass() override;

        /* We'll use different method for updating different type of buffer */
        /* for STATIC we create a staging buffer to transfer */
        /* for PERFRAME we just map data */
        /* for PERDRAW we use linear allocation with buffer mapping */
        void update(RenderResourceType type, Handle handle, u32 offset,
                    u32 size, void *data) override;
        void update(TextureHandle handle, u32 layer, u32 offx, u32 offy, u32 w,
                    u32 h, void *data) override;
        void update_texture_sampler(TextureHandle handle, u32 layer,
                                            const SamplerProperty &property) override;

        void *map_buffer(RenderResourceType type, Handle handle) override;
        void *map_texture(TextureHandle handle) override;
        void bind_depth_attachment(RenderPassHandle handle,
                                   TextureHandle texture, u32 face) override;
        void bind_color_attachment(RenderPassHandle handle, u8 slot,
                                   TextureHandle texture, u32 face) override;
        void dealloc(RenderResourceType type, Handle handle) override;

        void process_commands(std::deque<RenderCommand> &cmd_queue) override;
        void swap_buffer() override;
};

}  // namespace Seed

#endif