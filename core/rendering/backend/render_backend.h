#ifndef _SEED_RENDER_BACKEND_H_
#define _SEED_RENDER_BACKEND_H_
#include "core/rendering/render_common.h"
#include "core/rendering/rhi/render_command.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/handle.h"
#include "core/allocator/linear_allocator.h"
#include "core/rendering/shader_layout.h"
#include <shared_mutex>
#include <deque>

namespace Seed {
class RenderBackend;
class RenderCommandQueue {
        friend RenderBackend;

    private:
        std::deque<RenderCommand> cmd_queue;
        LinearAllocator data_pool;
        std::shared_mutex queue_lock;

    public:
        void push(RenderCommand &cmd) { this->cmd_queue.push_back(cmd); }
        void *alloc(u64 size, void *data = nullptr) {
            return this->data_pool.alloc(size, data);
        }
};

enum class RenderBackendType { VULKAN, XR_VULKAN };
class RenderEngine;
class RenderBackend {
    protected:
        inline static const u32 FRAMES_IN_FLIGHT = 3;
        RenderCommandQueue cmd_queue[FRAMES_IN_FLIGHT];
        std::atomic<int> current_frame = 0;
        u32 get_current_frame_index() {
            return current_frame % FRAMES_IN_FLIGHT;
        }

    public:
        RenderBackend() = default;
        ~RenderBackend() = default;
        virtual RenderBackendType get_type() = 0;
        virtual TextureHandle alloc_texture(TextureType type, u32 w, u32 h,
                                            PixelFormat format,
                                            MSAAType msaa_type,
                                            const SamplerProperty &property,
                                            const void *data) = 0;
        virtual TextureHandle alloc_textures(
            TextureType type, u32 w, u32 h, PixelFormat format, u32 count,
            const SamplerProperty &property) = 0;
        virtual TextureHandle alloc_cubemap(
            u32 w, u32 h, PixelFormat format,
            const SamplerProperty &property) = 0;

        virtual TextureHandle alloc_mappable_texture(
            TextureType type, u32 w, u32 h, PixelFormat format,
            const SamplerProperty &property, const void *data) = 0;
        virtual void query_texture_size(TextureHandle handle, u32 *w,
                                        u32 *h) = 0;
        virtual VertexHandle alloc_vertex(u32 stride, u32 element_cnt,
                                          UpdateFrequence frequence,
                                          const void *data) = 0;
        virtual IndexHandle alloc_indices(IndexType type, u32 element_cnt,
                                          UpdateFrequence frequence,
                                          const void *data) = 0;
        virtual ConstantHandle alloc_constant(u32 size, const void *data,
                                              UpdateFrequence frequence) = 0;
        virtual SSBOHandle alloc_storage_buffer(u32 size, const void *data,
                                                UpdateFrequence frequence) = 0;
        virtual ShaderHandle alloc_shader(
            const std::string &vertex_code, const std::string &fragment_code,
            const std::string &geometry_code, const std::string &tess_ctrl_code,
            const std::string &tess_eval_code) = 0;
        virtual void setup_shader_layout(ShaderHandle handle,
                                         const ShaderLayout &layout) = 0;

        virtual PipelineHandle alloc_pipeline(
            ShaderHandle shader, const RenderRasterizerState &rst_state,
            const RenderDepthStencilState &depth_state,
            const RenderBlendState &blend_state) = 0;
        virtual RenderPassHandle alloc_render_pass() = 0;
        virtual void copy_texture(TextureHandle dst, u32 dst_layer, TextureHandle src, u32 src_layer) = 0;
        virtual void update(RenderResourceType type, Handle handle, u32 offset,
                            u32 size, void *data) = 0;
        virtual void update(TextureHandle handle, u32 layer, u32 offx, u32 offy,
                            u32 w, u32 h, void *data) = 0;
        virtual void update_texture_sampler(
            TextureHandle handle, u32 layer,
            const SamplerProperty &property) = 0;
        virtual void *map_buffer(RenderResourceType type, Handle handle) = 0;
        virtual void *map_texture(TextureHandle handle) = 0;
        virtual void bind_depth_attachment(RenderPassHandle handle,
                                           TextureHandle texture, u32 face) = 0;
        virtual void bind_color_attachment(RenderPassHandle handle, u8 slot,
                                           TextureHandle texture, u32 face) = 0;
        virtual void dealloc(RenderResourceType type, Handle handle) = 0;

        virtual void process_commands(std::deque<RenderCommand> &cmd_queue) = 0;
        virtual void swap_buffer() = 0;

        void *alloc(u64 size = 0, void *data = nullptr);

        void *push_cmd(RenderCommand &cmd, u64 size = 0, void *data = nullptr);

        void process();
};

}  // namespace Seed

#endif