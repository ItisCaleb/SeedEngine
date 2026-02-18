#ifndef _SEED_DEDICATED_THREAD
#define _SEED_DEDICATED_THREAD
#include <thread>
#include <functional>
#include <string>

namespace Seed {
class DedicatedThread {
        typedef std::function<void()> Entrypoint;
    private:
        void set_thread_name(const std::string &name);
        std::thread th;
        static void thread_func(Entrypoint entry, bool is_coroutine);
        Entrypoint entry;
    public:
        static thread_local bool is_coroutine;
        DedicatedThread(const std::string &name, Entrypoint entry,
                        bool is_coroutine = false);
};
}  // namespace Seed

#endif