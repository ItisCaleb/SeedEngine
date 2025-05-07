#include "thread_pool.h"
#include <spdlog/spdlog.h>

namespace Seed {

ThreadPool *ThreadPool::get_instance() { return instance; }

void ThreadPool::thread_func(ThreadData *td) {
    while (!td->exit) {
        std::unique_lock lock(td->mutex);
        while (td->queue.is_empty()) {
            td->cv.wait(lock);
        }
        Work &w = td->queue.peek();
        td->queue.pop();
        lock.unlock();
        w.func(w.user_data);
    }
}

ThreadPool::ThreadData *ThreadPool::select_worker() {
    ThreadData *td = this->threads[this->last_worker + 1];
    this->last_worker = (this->last_worker + 1) % this->threads.size();
    return td;
}

void ThreadPool::add_work(UserFunc func, void *user_data) {
    Work w;
    w.func = func;
    w.user_data = user_data;
    ThreadData *t = this->select_worker();
    {
        std::lock_guard lk = std::lock_guard(t->mutex);
        t->queue.push(w);
    }
    t->cv.notify_one();
}

ThreadPool::ThreadPool(u32 thread_cnt) {
    instance = this;
    spdlog::info("Initializing Thread pool with thread count '{}'", thread_cnt);
    for (i32 i = 0; i < thread_cnt; i++) {
        this->threads.push_back(new ThreadData());
    }
}

ThreadPool::~ThreadPool() {}
}  // namespace Seed