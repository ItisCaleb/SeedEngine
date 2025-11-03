#include "os.h"
#include <thread>
#include <chrono>
namespace Seed {
#define PERIOD 1
#define TOLERANCE 0.02

void OS::delay(f32 seconds) {
    auto t0 = std::chrono::high_resolution_clock::now();
    auto target = t0 + std::chrono::nanoseconds(i64(seconds * 1e9));

    // sleep
    f64 ms = seconds * 1000 - (PERIOD + TOLERANCE);
    i32 ticks = (i32)(ms / PERIOD);
    if (ticks > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(ticks * PERIOD));

    // spin
    while (std::chrono::high_resolution_clock::now() < target)
        std::this_thread::yield();
}
u32 OS::cpu_count() { return std::thread::hardware_concurrency(); }
}  // namespace Seed