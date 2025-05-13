#include "linear_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "core/math/utils.h"

namespace Seed {
void *LinearAllocator::alloc(u64 size) {
    if (this->cur + size > this->cap) {
        /* we need to store data when the capacity is not enough */
        void *buf = malloc(size);
        this->tmp_bufs.push_back(buf);
        this->overflow_size += (this->cur + size) - this->cap;
        return buf;
    } else {
        this->cur += size;
        return (void *)((u64)this->memory_base + this->cur - size);
    }
}

void *LinearAllocator::alloc_data(u64 size, void *data) {
    void *ptr = this->alloc(size);
    memcpy(ptr, data, size);
    return ptr;
}
void LinearAllocator::free_all() {
    /* free the tmp buffers and realloc */
    if (overflow_size > 0) {
        for (void *buf : tmp_bufs) {
            free(buf);
        }
        tmp_bufs.clear();
        this->cap = roundup_to_pow2(this->cap + this->overflow_size);
        this->memory_base = realloc(this->memory_base, this->cap);
        this->overflow_size = 0;
    }
    this->cur = 0;
}
LinearAllocator::LinearAllocator() {
    this->memory_base = malloc(1024);
    this->cap = 1024;
}
LinearAllocator::~LinearAllocator() { free(this->memory_base); }
}  // namespace Seed