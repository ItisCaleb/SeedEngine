#ifndef _SEED_UUID_H_
#define _SEED_UUID_H_
#include "core/container/kstring.h"
#include "core/misc/hash.h"
#include "core/types.h"
#include "core/misc/random.h"
#include <functional>
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

        static UUID from_string(const KStr &str);

        bool operator==(const UUID &o) const {
            return this->data[0] == o.data[0] && this->data[1] == o.data[1];
        }

        bool operator<(const UUID &o) const {
            return this->data[1] < o.data[1] ||
                   (this->data[1] == o.data[1] && this->data[0] < o.data[0]);
        }

        bool is_null() const { return data[0] == 0 && data[1] == 0; }

        std::string to_string() const;
};

}  // namespace Seed
template <>
struct std::hash<Seed::UUID> {
        size_t operator()(const Seed::UUID &uuid) const noexcept {
            return Seed::Hash::hash_from_buffer(&uuid, sizeof(uuid));
        }
};
#endif