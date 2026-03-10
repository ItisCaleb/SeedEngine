#ifndef _SEED_UUID_H_
#define _SEED_UUID_H_
#include "core/types.h"
#include "core/misc/random.h"
#include <string>

namespace Seed {
class UUID {
    private:
        u64 data[2] = {};

    public:
        static UUID generate() {
            UUID uuid;
            Random rd;
            uuid.data[0] = rd.get_u64();
            uuid.data[1] = rd.get_u64();
            /* set version to 4 */
            uuid.data[0] &= ~(0b11110000ULL << 8);
            uuid.data[0] |= (0b01000000ULL << 8);
            /* set bits 7 and 6 of clock_seq_hi_and_reserved to one and zero */
            uuid.data[1] &= ~(0b11000000ULL << 56);
            uuid.data[1] |= (0b10000000ULL << 56);
            return uuid;
        }

        bool operator==(const UUID &o) const {
            return this->data[0] == o.data[0] && this->data[1] == o.data[1];
        }

        std::string to_string();
};

}  // namespace Seed

#endif