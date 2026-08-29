#ifndef _SEED_KCONTEXT_H_
#define _SEED_KCONTEXT_H_
#include <mmintrin.h>

namespace Seed{
    struct KContext {
#if defined(_M_X64) || defined(__x86_64__)
#ifdef _WIN32
        void *rbx;
        void *rbp;
        void *rdi;
        void *rsi;
        void *r12, *r13, *r14, *r15;
        void *rip;
        void *user_data;
        __m128i xmm6, xmm7, xmm8, xmm9, xmm10, xmm11, xmm12, xmm13, xmm14,
            xmm15;
#else
        static_assert(false, "Fiber for current architecture not implemented.");
#endif
#else
        static_assert(false, "Fiber for current architecture not implemented.");
#endif
};
}

extern "C" void* _jump_context(void *new_context, void *data);


#endif