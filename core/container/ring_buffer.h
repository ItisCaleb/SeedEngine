#ifndef _SEED_RING_BUFFER_H_
#define _SEED_RING_BUFFER_H_
#include "core/types.h"
#include <vector>

namespace Seed {
template <typename T>
class RingBuffer {
    private:
        u32 cap;
        u32 head = 0, tail = 0;
        std::vector<T> data;

    public:
        u32 size() { return (tail - head + cap) % cap; }

        bool is_empty() { return this->size() == 0; }

        bool is_full() { return this->size() == (this->cap - 1); }
        T* push(const T &element) {
            if (this->is_full()) {
                throw std::runtime_error("Ring buffer is full.");
            }
            data[tail] = element;
            T* tmp = &data[tail];
            tail = (tail + 1) % cap;
            return tmp;
        }

        T &peek() {
            if (this->is_empty()) {
                throw std::runtime_error("Ring buffer is empty.");
            }
            return data[head];
        }

        void pop() {
            if (this->is_empty()) {
                return;
            }
            head = (head + 1) % cap;
        }

        RingBuffer(u32 cap) : cap(cap) { this->data.resize(cap); }
        RingBuffer():RingBuffer(32){}
        RingBuffer(RingBuffer &&rb)
            : data(std::move(rb.data)),
              cap(rb.cap),
              head(rb.head),
              tail(rb.tail) {}
        ~RingBuffer() = default;
};
}  // namespace Seed

#endif