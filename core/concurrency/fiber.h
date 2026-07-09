#ifndef _SEED_FIBER_H_
#define _SEED_FIBER_H_
#include <mmintrin.h>
#include "core/types.h"
#include <functional>

namespace Seed {
enum class FiberStatus : u8 { READY, RUNNING, WAITING, DONE };

struct FiberContext {
#if defined(_M_X64) || defined(__x86_64__)
#ifdef _WIN32
        void *rbx;
        void *rbp;
        void *rdi;
        void *rsi;
        void *r12, *r13, *r14, *r15;
        void *rip;
        __m128i xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14, xmm15;
#else
        static_assert(false, "Fiber for current architecture not implemented.");
#endif
#else
        static_assert(false, "Fiber for current architecture not implemented.");
#endif
};

class Fiber {
    private:
        u8 stack[4096];
        std::function<void()> fiber_func;
        FiberStatus status;
        FiberContext context;
        static void start_func(Fiber *fiber);

    public:
        Fiber(std::function<void()> fiber_func);
        ~Fiber();
        void resume();
        void context_switch(Fiber &f);
};

}  // namespace Seed

#endif