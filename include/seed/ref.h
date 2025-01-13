#ifndef _SEED_REF_H_
#define _SEED_REF_H_
#include <seed/types.h>

#include <atomic>

namespace Seed {
template <typename T>
class Ref;
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
   private:
    T *data;

   public:
    template <typename... Args>

    T *operator*() {
        return data;
    }

    T *operator->() { return data; }

    bool operator==(const Ref &other) { return this->data == other.data; }

    void operator=(T *from) {
        if (from) {
            this->data = from;
            if (this->data->get_rc() == 0) {
                this->data->init_ref();
            } else {
                this->data->ref();
            }
        }
    }

    void operator=(const Ref &from) { this->operator=(from.data); }

    inline bool is_valid() const { return data != nullptr; }
    inline bool is_null() const { return data == nullptr; }

    Ref(T *data) { this->operator=(data); }

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

};  // namespace Seed

#endif