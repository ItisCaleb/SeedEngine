#include "uuid.h"
#include <fmt/format.h>

namespace Seed {
std::string UUID::to_string() {
    u64 time_low = (this->data[0] >> 32) & 0Xffffffff;
    u64 time_mid = (this->data[0] >> 16) & 0xffff;
    u64 time_hi_and_version = (this->data[0]) & 0xffff;
    u64 clock_seq = (this->data[1] >> 48) & 0xffff;
    u64 node = (this->data[1]) & 0Xffffffffffff;

    return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", time_low,
                       time_mid, time_hi_and_version, clock_seq, node);
}
}  // namespace Seed