#ifndef _SEED_THREAD_POOL_H_
#define _SEED_THREAD_POOL_H_
#include <vector>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include "core/types.h"
#include "core/container/ring_buffer.h"
#include "core/container/freelist.h"

namespace Seed {
typedef u32 WorkId;
typedef u32 GroupId;

class ThreadPool {
        typedef std::function<void(void *user_data)> UserFunc;

    private:
        struct Work {
                WorkId id;
                UserFunc func;
                void *user_data;
                bool is_completed = true;
                u32 thread_index;
        };
        struct Group {
                std::vector<WorkId> works;
        };
        struct ThreadData;
        std::vector<ThreadData *> threads;

        // Use to track workds
        FreeList<Work *> work_list;
        FreeList<Group> group_list;
        static void thread_func(ThreadPool *pool, ThreadData *td);
        u32 last_worker = -1;
        ThreadData *select_worker();
        struct ThreadData {
                u32 index;
                std::thread th;
                std::condition_variable cv;
                std::mutex mutex;
                RingBuffer<Work> queue;
                bool exit = false;
                ThreadData(ThreadPool *pool, u32 index)
                    : index(index), th(thread_func, pool, this), queue(256) {}
        };

    public:
        WorkId add_work(UserFunc func, void *user_data = nullptr);
        GroupId add_group(UserFunc func, std::vector<void *> &user_datas);
        void wait(WorkId id);
        void wait_group(GroupId id);
        ThreadPool(u32 thread_cnt);
        ~ThreadPool();
};
}  // namespace Seed

#endif