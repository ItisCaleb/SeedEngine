#include "os.h"
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif
namespace Seed {

void OS::delay(f32 seconds) {
#ifdef _WIN32
    Sleep((u32)(seconds * 1000));
#else
    usleep((u32)(seconds * 1000000));
#endif
}
u32 OS::cpu_count() { return std::thread::hardware_concurrency(); }
}  // namespace Seed