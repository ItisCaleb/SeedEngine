#ifndef _SEED_REF_H_
#define _SEED_REF_H_
#include "types.h"

#include <atomic>

namespace Seed {

class RefCounted {
   private:
    std::atomic_uint rc;

   public:
    void init_ref() { rc = 1; }

    bool ref() {
        if (rc == 0) return false;
        rc++;
        return true;
    }

    bool unref() {
        if (rc == 0) return false;
        rc--;
        return rc == 0;
    }

    u32 get_rc() { return rc; }
};

template <typename T>
class Ref {
    static_assert(std::is_base_of<RefCounted, T>::value,
                  "T must be a derived class of RefCounted in Ref<T>.");

   private:
   public:
    T *data = nullptr;
    T *operator*() { return data; }

    T *operator->() { return data; }

    bool operator==(const Ref &other) { return this->data == other.data; }

    void operator=(T *from) {
        if (from) {
            if (this->data == from) return;
            this->data = from;
            if (this->data->get_rc() == 0) {
                this->data->init_ref();
            } else {
                this->data->ref();
            }
        }
    }

    T *ptr() { return data; }

    void operator=(const Ref &from) { this->operator=(from.data); }

    inline bool is_valid() const { return data != nullptr; }
    inline bool is_null() const { return data == nullptr; }

    Ref(T *data) { this->operator=(data); }

    template <typename... Args>
    void create(const Args &...args) {
        this->operator=(new T(args...));
    }

    template <typename... Args>
    Ref(const Args &...args) {
        this->operator=(new T(args...));
    }
    Ref(const Ref &from) { this->operator=(from); }
    Ref() = default;
    ~Ref() {
        if (data) {
            if (data->unref()) {
                delete data;
            }
        }
    }
};


template <typename T, typename K>
inline Ref<T> ref_cast(Ref<K> ref) noexcept {
    return Ref<T>(static_cast<T*>(ref.data));
}
};  // namespace Seed

#endif