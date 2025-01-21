#ifndef _SEED_LINEAR_ALLOCATOR_H_
#define _SEED_LINEAR_ALLOCATOR_H_
#include "core/types.h"

namespace Seed {
class LinearAllocator {
   private:
    void *memory_base;
    u64 cur;
    u64 cap;

   public:
    void *alloc(u64 size);
    void free_all();
    LinearAllocator();
    ~LinearAllocator();
};
}  // namespace Seed

#endif