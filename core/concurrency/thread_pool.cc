#include "thread_pool.h"
#include <spdlog/spdlog.h>

namespace Seed {

void ThreadPool::thread_func(ThreadPool *pool, ThreadData *td) {
    while (!td->exit) {
        std::unique_lock lock(td->mutex);
        while (td->queue.is_empty()) {
            td->cv.wait(lock);
        }
        Work &w = td->queue.peek();
        td->queue.pop();
        lock.unlock();
        w.func(w.user_data);
        w.is_completed = true;
        pool->work_list.erase(w.id);
        td->cv.notify_one();
    }
}

ThreadPool::ThreadData *ThreadPool::select_worker() {
    u32 next_worker = (this->last_worker + 1) % this->threads.size();
    ThreadData *td = this->threads[next_worker];
    this->last_worker = next_worker;
    return td;
}

WorkId ThreadPool::add_work(UserFunc func, void *user_data) {
    Work w;
    w.func = func;
    w.user_data = user_data;
    ThreadData *t = this->select_worker();
    w.thread_index = t->index;
    w.is_completed = false;
    Work *new_work;
    {
        std::lock_guard lk = std::lock_guard(t->mutex);
        new_work = t->queue.push(w);
        u32 i = this->work_list.insert(new_work);
        new_work->id = i;
    }
    t->cv.notify_one();
    return new_work->id;
}

GroupId ThreadPool::add_group(UserFunc func, std::vector<void *> &user_datas) {
    u32 group_id = this->group_list.insert({});
    Group &g = this->group_list[group_id];
    for (void *data : user_datas) {
        WorkId work_id = this->add_work(func, data);
        g.works.push_back(work_id);
    }
    return group_id;
}

void ThreadPool::wait(WorkId id) {
    if (work_list.present(id)) {
        Work *w = work_list[id];
        if (w->is_completed) {
            return;
        }
        ThreadData *td = this->threads[w->thread_index];
        {
            std::unique_lock lock(td->mutex);
            td->cv.wait(lock);
        }
    }
}

void ThreadPool::wait_group(GroupId id) {
    if (group_list.present(id)) {
        Group &group = group_list[id];
        for (WorkId wid : group.works) {
            this->wait(wid);
        }
    }
}

ThreadPool::ThreadPool(u32 thread_cnt) {
    spdlog::info("Initializing Thread pool with thread count '{}'", thread_cnt);
    for (i32 i = 0; i < thread_cnt; i++) {
        this->threads.push_back(new ThreadData(this, i));
    }
}

ThreadPool::~ThreadPool() {}
}  // namespace Seed