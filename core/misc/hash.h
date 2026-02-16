#ifndef _SEED_HASH_H_
#define _SEED_HASH_H_

#include <vector>
#include "core/types.h"

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
            memcpy(buffer + size, data, sizeof(T));
            size += sizeof(T);
        }
        void clear(){
            this->size = 0;
        }
        u64 digest();
};
}  // namespace Seed

#endif