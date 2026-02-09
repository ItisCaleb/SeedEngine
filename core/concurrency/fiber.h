#ifndef _SEED_FIBER_H_
#define _SEED_FIBER_H_
#include "core/types.h"
#include <functional>

namespace Seed {
enum class FiberStatus : u8 { READY, RUNNING, WAITING, DONE };

struct FiberContext {
#if defined(_M_X64) || defined(__x86_64__)
#ifdef _WIN32
        u64 rbx;
        u64 rbp;
        u64 rdi;
        u64 rsi;
        u64 r12, r13, r14, r15;
        u64 rsp;
        u64 rip;
        __m128 xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
#else
        static_assert(false, "Fiber for current architecture not implemented.");
#endif
#else
        static_assert(false, "Fiber for current architecture not implemented.");
#endif
};

class Fiber {
    private:
        std::function<void> fiber_func;
        FiberStatus status;
        FiberContext context;
    public:
        Fiber(std::function<void> fiber_func)
            : fiber_func(fiber_func), status(FiberStatus::READY) {}
        ~Fiber();
};

}  // namespace Seed

#endif