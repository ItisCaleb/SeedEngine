#ifndef _SEED_KSTRING_H_
#define _SEED_KSTRING_H_
#include "core/types.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <vector>

namespace Seed {

const constexpr u8 UTF8_CODEPOINT_MASK = 0b10111111;
const constexpr u8 UTF8_ONE_CODEPOINT_MASK = 0b01111111;
const constexpr u8 UTF8_TWO_CODEPOINT_MASK = 0b11011111;
const constexpr u8 UTF8_THREE_CODEPOINT_MASK = 0b11101111;
const constexpr u8 UTF8_FOUR_CODEPOINT_MASK = 0b111110111;
/* UTF8 char */
struct KChar {
        u8 b[4];
        static const constexpr u8 _code_point_length(u8 c) {
            if ((c & UTF8_ONE_CODEPOINT_MASK) == c) return 1;
            if ((c & UTF8_TWO_CODEPOINT_MASK) == c) return 2;
            if ((c & UTF8_THREE_CODEPOINT_MASK) == c) return 3;
            if ((c & UTF8_FOUR_CODEPOINT_MASK) == c) return 4;
            return 0;
        };

        template <std::size_t N>
        constexpr KChar(const char (&s)[N]) : b() {
            static_assert(N <= 5, "KChar can only accept UTF-8 character.");
            const u32 length = _code_point_length(s[0]);
            if (length == 0 || length != N - 1)
                throw std::invalid_argument("Invalid UTF-8 leading byte.");
            for (u32 i = 0; i < length; i++) {
                b[i] = s[i];
            }
        }
        constexpr KChar(const char c) : b() {
            const u32 length = _code_point_length(c);
            if (length != 1)
                throw std::invalid_argument("Invalid UTF-8 leading byte.");
            b[0] = c;
        }
        bool operator==(char c) { return b[0] == c; }
        bool operator==(KChar c) {
            return b[0] == c.b[0] && b[1] == c.b[1] && b[2] == c.b[2] &&
                   b[3] == c.b[3];
        }

        u8 size() const { return _code_point_length(b[0]); }
};
class KStr;
/* UTF8 string */
class KString {
    private:
        /* string raw length with null byte*/
        u32 _size = 0;
        /* buffer total size */
        u32 _cap = 0;
        u8 *_data = nullptr;
        constexpr u8 _code_point_length(u8 c) {
            if ((c & UTF8_ONE_CODEPOINT_MASK) == c) return 1;
            if ((c & UTF8_TWO_CODEPOINT_MASK) == c) return 2;
            if ((c & UTF8_THREE_CODEPOINT_MASK) == c) return 3;
            if ((c & UTF8_FOUR_CODEPOINT_MASK) == c) return 4;
            return 0;
        };
        /* size is without null byte */
        void _append(const u8 *c, u32 size);

    public:
        KString() = default;
        KString(u32 cap);
        KString(char *str);
        KString(const char *str);

        KString(const std::string &str);
        KString(KString &&str);
        ~KString();
        KStr to_str();

        /* return utf8 length */
        u32 length();
        bool is_empty() { return this->_size == 0; }
        void push(const char c);
        void push(const KChar &c);
        void append(const KStr &str);
        void append(KString &str);

        void operator=(KString &str);
        void operator+=(const KStr &str) { append(str); }
        void operator+=(KString &str) { append(str); }

        /* return raw data size */
        size_t size() { return this->_size; }
        const u8 *data() { return this->_data; }

        /* utility methods */
        KString clone();
};

/* A raw view to string data */
/* Do not append original string while a KStr holding it, */
/* or the reallocation will invalidate this. */
class KStr {
    private:
        const u8 *_data = nullptr;
        u32 _length = 0;
        KStr(u8 *data, u32 length) : _data(data), _length(length) {};

    public:
        KStr() = default;
        KStr(KString &str);
        constexpr KStr(const char *str) {
            _data = (u8 *)str;
            for (u32 i = 0; str[i] != '\0'; i++) {
                _length++;
            }
        }
        KString string();
        const u8 *data() const { return _data; }
        u32 length() const { return _length; }
        bool operator==(const KStr &str) const;
        bool start_with(const KStr &str) const;
        bool end_with(const KStr &str) const;
        bool contains(const KStr &str) const;

        /* return -1 if not found */
        i32 find_first(const KStr &pattern) const;

        /* return -1 if not found */
        i32 find_last(const KStr &pattern) const;
        std::vector<u32> find_all(const KStr &pattern) const;
        std::vector<KStr> split(const KStr &pattern) const;
        std::vector<KStr> split_n(const KStr &pattern, u32 n) const;
        KString replace(const KStr &pattern, const KStr &to);
        KString replace_n(const KStr &pattern, const KStr &to, u32 n);

        KStr ltrim() const;
        KStr rtrim() const;
        KStr trim() const;
};

};  // namespace Seed

#endif