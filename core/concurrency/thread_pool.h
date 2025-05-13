#ifndef _SEED_THREAD_POOL_H_
#define _SEED_THREAD_POOL_H_
#include <vector>
#include <thread>
#include "core/types.h"
#include "core/container/ring_buffer.h"
#include "core/container/freelist.h"

namespace Seed {
typedef u32 WorkId;
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
        struct ThreadData;
        std::vector<ThreadData *> threads;
        FreeList<Work *> work_list;
        inline static ThreadPool *instance = nullptr;
        static void thread_func(ThreadData *td);
        u32 last_worker = -1;
        ThreadData *select_worker();
        struct ThreadData {
                u32 index;
                std::thread th;
                std::condition_variable cv;
                std::mutex mutex;
                RingBuffer<Work> queue;
                bool exit = false;
                ThreadData(u32 index) : index(index), th(thread_func, this) {}
        };

    public:
        static ThreadPool *get_instance();

        WorkId add_work(UserFunc func, void *user_data);
        void wait(WorkId id);
        ThreadPool(u32 thread_cnt);
        ~ThreadPool();
};
}  // namespace Seed

#endif