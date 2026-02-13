#include "render_backend.h"

namespace Seed {
void *RenderBackend::alloc(u64 size, void *data) {
    RenderCommandQueue &queue = this->cmd_queue[current_queue & 1];
    queue.queue_lock.lock();
    void *_data = queue.data_pool.alloc(size, data);
    queue.queue_lock.unlock();
    return _data;
}

void *RenderBackend::push_cmd(RenderCommand &cmd, u64 size, void *data) {
    RenderCommandQueue &queue = this->cmd_queue[current_queue & 1];
    queue.queue_lock.lock();
    if (size > 0) {
        cmd.data = queue.data_pool.alloc(size, data);
    }
    queue.cmd_queue.push_back(cmd);
    queue.queue_lock.unlock();
    return cmd.data;
}

void RenderBackend::process() {
    {
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
}
}  // namespace Seed