#ifndef _SEED_RANDOM_H_
#define _SEED_RANDOM_H_
#include "core/types.h"
#include <random>

namespace Seed {
class Random {
    private:
        inline static u64 g_seed = 0;
        inline static std::mt19937_64 g_generator;

    public:
        Random() {
            if (g_seed == 0) {
                std::random_device rd;
                g_seed = rd();
                g_generator.seed(g_seed);
            }
        }

        i64 get_i64() { return (i64)g_generator(); }
        u64 get_u64() { return g_generator(); }
};
};  // namespace Seed

#endif