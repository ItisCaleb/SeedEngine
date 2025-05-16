#include "linear_allocator.h"
#include <stdlib.h>
#include <string.h>
#include "core/math/utils.h"
#include <spdlog/spdlog.h>

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
        spdlog::debug("Linear allocator resize to '{}'", this->cap);
    }
    this->cur = 0;
}
LinearAllocator::LinearAllocator() {
    u64 size = 1024 * 1024 * 16;
    this->memory_base = malloc(size);
    this->cap = size;
}
LinearAllocator::~LinearAllocator() { free(this->memory_base); }
}  // namespace Seed