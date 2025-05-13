#ifndef _SEED_LINEAR_ALLOCATOR_H_
#define _SEED_LINEAR_ALLOCATOR_H_
#include "core/types.h"
#include <vector>

namespace Seed {
class LinearAllocator {
    private:
        void *memory_base;
        u64 cur;
        u64 cap;
        std::vector<void *> tmp_bufs;
        u64 overflow_size = 0;

    public:
        void *alloc(u64 size);

        template <typename T, typename... Args>
        T *alloc_new(const Args &...args) {
            void *ptr = alloc(sizeof(T));
            return new (ptr) T(args...);
        }

        void *alloc_data(u64 size, void *data);
        void free_all();
        LinearAllocator();
        ~LinearAllocator();
};
}  // namespace Seed

#endif