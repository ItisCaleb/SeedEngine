#ifndef _SEED_HASH_H_
#define _SEED_HASH_H_

#include "core/types.h"
#include <cassert>
#include <string_view>

namespace Seed {
constexpr const u32 HASH_BUFFER_SIZE = 256;
class Hash {
    private:
        u32 size = 0;
        u8 buffer[HASH_BUFFER_SIZE];

    public:
        Hash() = default;
        template <typename T>
        void update(T *data){
            assert(size + sizeof(T) < HASH_BUFFER_SIZE);
            memcpy(buffer + size, data, sizeof(T));
            size += sizeof(T);
        }
        void clear(){
            this->size = 0;
        }
        u64 digest();
};

inline constexpr uint64_t fnv1a(std::string_view s) {
    uint64_t h = 14695981039346656037ULL;
    for (char c : s)
        h = (h ^ static_cast<uint64_t>(c)) * 1099511628211ULL;
    return h;
}
}  // namespace Seed

#endif