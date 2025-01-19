#ifndef _SEED_LINEAR_ALLOCATOR_H_
#define _SEED_LINEAR_ALLOCATOR_H_
#include "core/types.h"

namespace Seed {
class LinearAllocator {
   private:
    void *memory_base;
    size_t cur;
    size_t cap;

   public:
    void *alloc(size_t size);
    void free_all();
    LinearAllocator();
    ~LinearAllocator();
};
}  // namespace Seed

#endif