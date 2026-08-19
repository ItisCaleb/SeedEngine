#ifndef _SEED_SPARSE_SET_H_
#define _SEED_SPARSE_SET_H_
#include "core/types.h"
#include <vector>

namespace Seed {
template <typename T>
class SparseSet {
    private:
        std::vector<i32> sparse;
        std::vector<T> dense;
        i32 last_element = -1;

    public:
        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        i32 insert(const T &element) {
            i32 index = this->dense.size();
            this->dense.push_back(element);
            sparse.push_back(index);
            last_element++;
            return index;
        }

        void erase(i32 index) {
            if (index < 0 || index >= this->sparse.size()) {
                return;
            }
            i32 _index = this->sparse[index];
            if (_index == -1) return;
            if (_index != this->last_element) {
                this->dense[_index] = this->dense[this->last_element];
                this->sparse[this->last_element] = _index;
            } else {
                this->sparse.pop_back();
            }
            this->sparse[index] = -1;
            this->last_element--;
        }

        T *get_or_null(i32 index) {
            if (index < 0 || index >= this->sparse.size()) {
                return nullptr;
            }
            u32 _index = this->sparse[index];
            if (_index == -1) return nullptr;
            return &this->dense[_index];
        }

        void clear() {
            this->sparse.clear();
            this->dense.clear();
            this->last_element = -1;
        }

        // Returns the nth element.
        T &operator[](int n) {
            T *data = get_or_null(n);
            if (!data) {
                throw std::runtime_error("Accessing invalid sparse set element");
            }
            return *data;
        }

        // Returns the nth element.
        const T &operator[](int n) const {
            T *data = get_or_null(n);
            if (!data) {
                throw std::runtime_error("Accessing invalid sparse set element");
            }
            return *data;
        }

        T *data() { return this->dense.data(); }

        u32 size() { return last_element + 1; }

        iterator begin() { return dense.begin(); }
        iterator end() {
            return last_element < 0 ? dense.begin()
                                    : dense.begin() + last_element + 1;
        }
        const_iterator begin() const { return dense.begin(); }
        const_iterator end() const {
            return last_element < 0 ? dense.begin()
                                    : dense.begin() + last_element + 1;
        }
};
}  // namespace Seed

#endif
