#ifndef _SEED_LINEAR_ALLOCATOR_H_
#define _SEED_LINEAR_ALLOCATOR_H_
#include "core/types.h"
#include <vector>
#include <mutex>

namespace Seed {
class LinearAllocator {
    private:
        void *memory_base;
        u64 cur;
        u64 cap;
        std::vector<void *> tmp_bufs;
        u64 overflow_size = 0;
        std::mutex mu;

    public:
        void *alloc(u64 size, void *data = nullptr);
        void free_all();
        LinearAllocator();
        ~LinearAllocator();
};
}  // namespace Seed

#endif