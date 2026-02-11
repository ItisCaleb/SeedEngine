#ifndef _SEED_RENDER_BACKEND_H_
#define _SEED_RENDER_BACKEND_H_
#include "render_command.h"
#include "render_resource.h"
#include "core/handle.h"
#include "core/allocator/linear_allocator.h"
#include <shared_mutex>
#include <algorithm>
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

enum class RenderBackendType { OPENGL, VULKAN };

class RenderBackend {
    protected:
        RenderCommandQueue cmd_queue[2];
        RenderResource current_pipeline;
        std::atomic<int> current_queue = 0;
        std::shared_mutex queue_lock;

    public:
        RenderBackend() = default;
        ~RenderBackend() = default;
        virtual RenderBackendType get_type() = 0;
        virtual void alloc_texture(RenderResource *rc, TextureType type, u32 w,
                                   u32 h, PixelFormat format,
                                   const SamplerProperty &property) = 0;
        virtual void alloc_vertex(RenderResource *rc, u32 stride,
                                  u32 element_cnt) = 0;

        virtual void alloc_indices(RenderResource *rc, IndexType type,
                                   u32 element_cnt) = 0;
        virtual void alloc_shader(RenderResource *rc,
                                  const std::string &vertex_code,
                                  const std::string &fragment_code,
                                  const std::string &geometry_code,
                                  const std::string &tesselation_code,
                                  const std::string &tess_eval_code) = 0;
        virtual void alloc_constant(RenderResource *rc, u32 size) = 0;
        virtual void alloc_pipeline(RenderResource *rc, RenderResource shader,
                                    const RenderRasterizerState &rst_state,
                                    const RenderDepthStencilState &depth_state,
                                    const RenderBlendState &blend_state) = 0;
        virtual void alloc_render_target(RenderResource *rc,
                                         bool depth_only) = 0;

        virtual void alloc_buffer(RenderResource *rc, u32 size) = 0;
        virtual void dealloc(RenderResource *r) = 0;
        virtual void process_commands(std::deque<RenderCommand> &cmd_queue) = 0;
        virtual void swap_buffer() = 0;

        void *alloc(u64 size = 0, void *data = nullptr) {
            RenderCommandQueue &queue = this->cmd_queue[current_queue & 1];
            queue.queue_lock.lock();
            void *_data = queue.data_pool.alloc(size, data);
            queue.queue_lock.unlock();
            return _data;
        }

        void *push_cmd(RenderCommand &cmd, u64 size = 0, void *data = nullptr) {
            RenderCommandQueue &queue = this->cmd_queue[current_queue & 1];
            queue.queue_lock.lock();
            if (size > 0) {
                cmd.data = queue.data_pool.alloc(size, data);
            }
            queue.cmd_queue.push_back(cmd);
            queue.queue_lock.unlock();
            return cmd.data;
        }

        void process() {
            RenderCommandQueue &queue = this->cmd_queue[current_queue & 1];
            current_queue++;
            queue.queue_lock.lock();
            std::stable_sort(queue.cmd_queue.begin(), queue.cmd_queue.end(),
                             RenderCommand::cmp);
            this->process_commands(queue.cmd_queue);
            queue.data_pool.free_all();
            queue.queue_lock.unlock();
            swap_buffer();
        }
};

}  // namespace Seed

#endif