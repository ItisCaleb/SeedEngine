#ifndef _SEED_OS_H_
#define _SEED_OS_H_
#include "core/types.h"

namespace Seed {

enum class OSType { WINDOWS, LINUX, MACOS };

class OS {
    public:
        static void delay(f32 seconds);
        static u32 cpu_count();
        static constexpr OSType get_os() {
#ifdef _WIN32
            return OSType::WINDOWS;
#elif __APPLE__
            return OSType::MACOS;
#else
            return OSType::LINUX;
#endif
        }
};
}  // namespace Seed

#endif