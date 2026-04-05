#include "hash.h"
#define XXH_INLINE_ALL
#define XXH_NO_STREAM
#include "thirdparty/xxhash/xxhash.h"

namespace Seed {

u64 Hash::digest() {
    XXH64_hash_t hash = XXH64(buffer, size, 0);
    return hash;
}

u64 Hash::hash_from_buffer(const void *data, u64 size) {
    return XXH64(data, size, 0);
}
}  // namespace Seed