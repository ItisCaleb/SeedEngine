#ifndef _SEED_FIBER_H_
#define _SEED_FIBER_H_
#include "core/types.h"
#include <functional>
#include "core/concurrency/kcontext.h"

namespace Seed {
enum class FiberStatus : u8 { READY, RUNNING, WAITING, DONE };

class Fiber {
    private:
        std::function<void(Fiber *)> entry_func;
        FiberStatus status = FiberStatus::READY;
        KContext *ctx = nullptr;
        KContext *yield_ctx = nullptr;
        u8 stack[4096];
        static void fiber_entry(void *old_context);
    public:
        Fiber(void (*entry_func)(Fiber *));
        void resume();
        void yield();
};

}  // namespace Seed

#endif