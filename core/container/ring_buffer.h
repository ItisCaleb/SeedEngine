#ifndef _SEED_RING_BUFFER_H_
#define _SEED_RING_BUFFER_H_
#include "core/types.h"
#include <vector>
#include <atomic>
#include <stdexcept>

namespace Seed {
template <typename T>
class RingBuffer {
    private:
        u32 cap;
        std::atomic<u32> head = 0, tail = 0;
        std::vector<T> data;

    public:
        template <typename K>
        class Iterator {
                std::vector<K> &data;
                u32 cap;
                u32 cur;

            public:
                Iterator(std::vector<K> &data, u32 cur, u32 cap)
                    : data(data), cur(cur), cap(cap) {}

                K &operator*() { return data[cur]; }
                K *operator->() { return &data[cur]; }

                Iterator<K> &operator++() {
                    cur = (cur + 1) % cap;
                    return *this;
                }
                Iterator<K> operator++(int) {
                    Iterator<K> tmp = *this;
                    cur = (cur + 1) % cap;
                    return tmp;
                }

                bool operator==(const Iterator<K> &other) const {
                    return data.data() == other.data.data() && cur == other.cur;
                }
                bool operator!=(const Iterator<K> &other) const {
                    return data.data() != other.data.data() || cur != other.cur;
                }
        };

        Iterator<T> begin() { return Iterator<T>(data, head, cap); }
        Iterator<T> end() { return Iterator<T>(data, tail, cap); }
        u32 size() { return (tail - head + cap) % cap; }

        bool is_empty() { return this->size() == 0; }

        bool is_full() { return this->size() == (this->cap - 1); }
        T *push(const T &element) {
            if (this->is_full()) {
                throw std::runtime_error("Ring buffer is full.");
            }
            data[tail] = element;
            T *tmp = &data[tail];
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
        void resize(u32 cap) {
            this->data.resize(cap);
            this->cap = cap;
        }
        RingBuffer(u32 cap) : cap(cap) { this->data.resize(cap); }
        RingBuffer() : RingBuffer(4096) {}
        RingBuffer(RingBuffer &&rb)
            : data(std::move(rb.data)),
              cap(rb.cap),
              head(rb.head),
              tail(rb.tail) {}
        ~RingBuffer() = default;
};
}  // namespace Seed

#endif