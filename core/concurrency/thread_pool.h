#ifndef _SEED_THREAD_POOL_H_
#define _SEED_THREAD_POOL_H_
#include <vector>
#include <thread>
#include "core/types.h"
#include "core/container/ring_buffer.h"

namespace Seed {
class ThreadPool {
        typedef std::function<void(void *user_data)> UserFunc;

    private:
        struct Work {
                UserFunc func;
                void *user_data;
        };
        struct ThreadData;
        std::vector<ThreadData *> threads;
        inline static ThreadPool *instance = nullptr;
        static void thread_func(ThreadData *td);
        u32 last_worker = -1;
        ThreadData* select_worker();
        struct ThreadData {
                std::thread th;
                std::condition_variable cv;
                std::mutex mutex;
                RingBuffer<Work> queue;
                bool exit = false;
                ThreadData() : th(thread_func, this) {}
        };

    public:
        static ThreadPool *get_instance();

        void add_work(UserFunc func, void *user_data);
        ThreadPool(u32 thread_cnt);
        ~ThreadPool();
};
}  // namespace Seed

#endif