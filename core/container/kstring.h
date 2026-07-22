#ifndef _SEED_KSTRING_H_
#define _SEED_KSTRING_H_
#include <fmt/base.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include "core/misc/hash.h"
#include "core/types.h"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Seed {

const constexpr char UTF8_CODEPOINT_MASK = 0b10111111;
const constexpr char UTF8_ONE_CODEPOINT_MASK = 0b01111111;
const constexpr char UTF8_TWO_CODEPOINT_MASK = 0b11011111;
const constexpr char UTF8_THREE_CODEPOINT_MASK = 0b11101111;
const constexpr char UTF8_FOUR_CODEPOINT_MASK = 0b111110111;
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
/* all methods will assume the buffer is valid utf8 string*/
class KString {
    private:
        /* string raw length without null byte*/
        u32 _size = 0;
        /* buffer total size */
        u32 _cap = 0;
        char *_data = nullptr;
        constexpr u8 _code_point_length(u8 c) const {
            if ((c & UTF8_ONE_CODEPOINT_MASK) == c) return 1;
            if ((c & UTF8_TWO_CODEPOINT_MASK) == c) return 2;
            if ((c & UTF8_THREE_CODEPOINT_MASK) == c) return 3;
            if ((c & UTF8_FOUR_CODEPOINT_MASK) == c) return 4;
            return 0;
        };
        /* size is without null byte */
        void _append(const char *c, u32 size);

    public:
        KString() = default;
        KString(u32 cap);
        KString(char *str);
        KString(const char *str);
        KString(const std::string &str);
        KString(const KStr &str);
        KString(const KString &str);
        KString(KString &&str) noexcept;
        ~KString();
        KStr to_str() const;

        /* return utf8 length */
        u32 length() const;
        bool is_empty() const { return this->_size == 0; }
        void push(const char c);
        void push(const KChar &c);
        void append(const KStr &str);
        void append(KString &str);
        void clear() {
            _size = 0;
            if (this->_data) {
                this->_data[0] = '\0';
            }
        }
        void resize(u32 size);

        /* pop utf8 chars */
        void pop(u32 utf8_count = 1);
        void pop_raw(u32 count);

        KString &operator=(const KString &other);
        KString &operator=(KString &&other) noexcept;

        void operator+=(const KStr &str) { append(str); }
        void operator+=(KString &str) { append(str); }
        bool operator==(const KString &other) const;
        bool operator<(const KString &str) const;

        /* return raw data size */
        size_t size() const { return this->_size; }
        const char *data() const { return this->_data; }

        /* utility methods */
        KString clone() const;
        KString &replace(KStr from, KStr to);
        KString &to_lower();
        KString &to_upper();
};

/* A raw view to string data */
/* Do not append original string while a KStr holding it, */
/* or the reallocation will invalidate this. */
class KStr {
    private:
        const char *_data = nullptr;
        u32 _length = 0;

    public:
        KStr() = default;
        KStr(const char *data, u32 length) : _data(data), _length(length) {};
        KStr(const KString &str);
        KStr(const KString &str, u32 length);
        constexpr KStr(const char *str) {
            _data = str;
            for (u32 i = 0; str[i] != '\0'; i++) {
                _length++;
            }
        }
        KStr(const std::string &str);

        KString string() const;
        const char *data() const { return _data; }
        const char *end() const { return &_data[_length]; }
        u32 length() const { return _length; }
        bool operator==(const KStr &str) const;
        bool operator!=(const KStr &str) const { return !operator==(str); }
        bool operator<(const KStr &str) const;
        char operator[](u32 idx) const;
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
        std::pair<KStr, KStr> split_at(u32 i) const;
        KString replace(const KStr &pattern, const KStr &to) const;
        KString replace_n(const KStr &pattern, const KStr &to, u32 n) const;

        KStr ltrim() const;
        KStr rtrim() const;
        KStr trim() const;

        bool is_empty() const { return _length == 0; }
};

template <typename T>
std::ostream &operator<<(std::ostream &out, const T &value) {
    ::std::operator<<(out, fmt::format("{}", value));
    return out;
}

};  // namespace Seed

template <>
struct std::hash<Seed::KString> {
        size_t operator()(const Seed::KString &p) const noexcept {
            return Seed::Hash::hash_from_buffer(p.data(), p.size());
        }
};

template <>
struct std::hash<Seed::KStr> {
        size_t operator()(const Seed::KStr &p) const noexcept {
            return Seed::Hash::hash_from_buffer(p.data(), p.length());
        }
};

template <>
struct fmt::formatter<Seed::KStr> : fmt::formatter<fmt::string_view> {
        constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

        constexpr auto format(const Seed::KStr &sv, format_context &ctx) const {
            // Cast or convert your type to fmt::string_view and delegate
            return formatter<fmt::string_view>::format(
                {(const char *)sv.data(), sv.length()}, ctx);
        }
};

template <>
struct fmt::formatter<Seed::KString> : fmt::formatter<fmt::string_view> {
        constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

        auto format(const Seed::KString &sv, format_context &ctx) const {
            // Cast or convert your type to fmt::string_view and delegate
            return formatter<fmt::string_view>::format(
                {(char *)sv.data(), sv.size()}, ctx);
        }
};

#endif
