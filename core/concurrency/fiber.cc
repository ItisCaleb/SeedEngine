#include "fiber.h"
#include "core/types.h"

namespace Seed {

void Fiber::start_func(Fiber *fiber) { fiber->fiber_func(); }

Fiber::Fiber(std::function<void()> fiber_func)
    : fiber_func(fiber_func), status(FiberStatus::READY) {
    context.rip = (void *)start_func;
    u64 sp = (u64)this->stack + sizeof(this->stack);
    /* preserve some space */
    sp -= 128;
    context.rsp = (void *)sp;
    context.rbp = context.rsp;
#if defined(_M_X64) || defined(__x86_64__)
#ifdef _WIN32

#endif
#endif
}

}  // namespace Seed