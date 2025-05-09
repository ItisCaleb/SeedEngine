#include "linear_allocator.h"
#include <stdlib.h>

namespace Seed {
void *LinearAllocator::alloc(u64 size) {
    this->cur += size;
    if (this->cur > this->cap) {
        this->cap *= 2;
        this->memory_base = realloc(this->memory_base, this->cap);
    }
    return (void *)((u64)this->memory_base + size);
}
void LinearAllocator::free_all() { this->cur = 0; }
LinearAllocator::LinearAllocator() {
    this->memory_base = malloc(1024);
    this->cap = 1024;
}
LinearAllocator::~LinearAllocator() { free(this->memory_base); }
}  // namespace Seed