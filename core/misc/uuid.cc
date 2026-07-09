#include "uuid.h"
#include <fmt/format.h>
#include <array>
#include "core/container/kstring.h"

namespace Seed {

static constexpr std::array<char, 256> kHexTable{[]() constexpr {
    std::array<char, 256> t{};
    for (int i = 0; i < 256; ++i) t[i] = -1;
    for (int i = 0; i < 10; ++i) t['0' + i] = i;
    for (int i = 0; i < 6; ++i) {
        t['a' + i] = 10 + i;
        t['A' + i] = 10 + i;
    }
    return t;
}()};

std::string UUID::to_string() const {
    u64 time_low = (this->data[0] >> 32) & 0Xffffffff;
    u64 time_mid = (this->data[0] >> 16) & 0xffff;
    u64 time_hi_and_version = (this->data[0]) & 0xffff;
    u64 clock_seq = (this->data[1] >> 48) & 0xffff;
    u64 node = (this->data[1]) & 0Xffffffffffff;

    return fmt::format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}", time_low,
                       time_mid, time_hi_and_version, clock_seq, node);
}

UUID UUID::from_string(const KStr &str) {
    if (str.length() != 36) return UUID{};

    auto parse_hex = [&](int pos, int len) -> u64 {
        u64 val = 0;
        for (int i = 0; i < len; ++i) {
            int8_t h = kHexTable[(uint8_t)str[pos + i]];
            if (h < 0) throw std::invalid_argument("invalid UUID char");
            val = (val << 4) | h;
        }
        return val;
    };

    /* "{:08x}-{:04x}-{:04x}-{:04x}-{:012x}" */
    /*   pos=0    9     14    19     24 */
    u64 time_low = parse_hex(0, 8);              // 32 bits
    u64 time_mid = parse_hex(9, 4);              // 16 bits
    u64 time_hi_and_version = parse_hex(14, 4);  // 16 bits
    u64 clock_seq = parse_hex(19, 4);            // 16 bits
    u64 node = parse_hex(24, 12);                // 48 bits

    UUID uuid;
    uuid.data[0] = (time_low << 32) | (time_mid << 16) | time_hi_and_version;
    uuid.data[1] = (clock_seq << 48) | node;
    return uuid;
}
}  // namespace Seed