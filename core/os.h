#ifndef _SEED_OS_H_
#define _SEED_OS_H_
#include "core/types.h"

namespace Seed {
class OS {
    public:
        static void delay(f32 seconds);
        static u32 cpu_count();
};
}  // namespace Seed

#endif