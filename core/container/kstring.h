#ifndef _SEED_KSTRING_H_
#define _SEED_KSTRING_H_
#include "core/types.h"
#include <string>

namespace Seed {
/* UTF8 char */
class KChar {
        u8 b[4];
};

/* UTF8 string, immutable */
class KString {
        const static u64 KSTRING_EXT_MASK = 0x8000000000000000;

    private:
        /* if MSB is set, then it's externel */
        u64 _size;
        const char *data;
        KString() = default;
    public:
        constexpr KString(const char *str) : _size(0), data(0) {
            this->operator=(str);
        }
        constexpr void operator=(const char *str) {
            u8 i = 0;
            for (; str[i] != '\0'; i++) {
            }
            data = str;
            _size = i;
            _size |= KSTRING_EXT_MASK;
        }

        void operator=(KString &str) {
            
        }

        KString(const std::string &str);
        ~KString();
        size_t size(){
            return this->_size & (~KSTRING_EXT_MASK);
        }
        size_t utf8_size();

        KString clone();
};

};  // namespace Seed

#endif