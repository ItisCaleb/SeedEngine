#include "kstring.h"
#include <fmt/base.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>
#include "core/types.h"
#include "core/io/path.h"

namespace Seed {

KString::KString(u32 cap) : _cap(cap), _size(0) {
    if (cap == 0) {
        return;
    }
    _data = (char *)malloc(_cap);
}
KString::KString(char *str) : KString((const char *)str) {}
KString::KString(const char *str) {
    _size = strlen(str);
    _cap = _size + 1;
    _data = (char *)malloc(_cap);
    memcpy(_data, str, _cap);
}

KString::KString(const KStr &str) {
    _size = str.length();
    _cap = _size + 1;
    _data = (char *)malloc(_cap);
    memcpy(_data, str.data(), _cap);
}

KString::KString(const std::string &str) {
    _size = str.length();
    _cap = _size + 1;
    _data = (char *)malloc(_cap);
    memcpy(_data, str.data(), _cap);
}

KString::KString(const KString &str) {
    if(str._cap == 0) return;
    _size = str.size();
    _cap = str._cap;
    _data = (char *)malloc(_cap);
    memcpy(_data, str.data(), _size + 1);
}

KString::KString(KString &&str) noexcept { operator=(str); }

KStr KString::to_str() const { return KStr(*this); }

KString &KString::operator=(const KString &str) {
    if (str._cap > _cap) {
        if (_data) {
            free(_data);
        }
        _cap = str._cap;
        _data = (char *)malloc(_cap);
    }
    _size = str._size;
    memcpy(_data, str._data, _size + 1);
    return *this;
}

KString &KString::operator=(KString &&other) noexcept {
    if (_data) {
        free(_data);
    }
    _data = other._data;
    _size = other._size;
    _cap = other._cap;
    other._data = nullptr;
    other._cap = 0;
    return *this;
}

bool KString::operator==(const KString &other) const {
    return to_str() == other.to_str();
}

bool KString::operator<(const KString &str) const {
    return to_str() < str.to_str();
}

KString::~KString() {
    if (_data) {
        free(_data);
    }
}

u32 KString::length() const {
    u32 length = 0;
    /* assert string is valid utf8 */
    for (u32 i = 0; i < _size; i++) {
        length++;
        i += _code_point_length(_data[i]);
    }
    return length;
}

void KString::_append(const char *c, u32 size) {
    if (_data == nullptr) {
        _size = 0;
        _cap = std::max(size + 1, 16u);
        _data = (char *)malloc(_cap);
    }
    if (_cap < _size + size + 1) {
        u32 new_size = (_cap + size + 1) << 1;
        void *tmp = realloc(_data, new_size);
        if (tmp == nullptr) {
            throw std::runtime_error("KString reallocation error.");
        }
        _cap = new_size;
        _data = (char *)tmp;
    }
    memcpy(&_data[_size], c, size);
    _size += size;
    _data[_size] = '\0';
}

void KString::push(const char c) { _append((const char *)&c, 1); }
void KString::push(const KChar &c) {
    _append((char *)c.b, c.size());
    return;
}

void KString::append(const KStr &str) { _append(str.data(), str.length()); }
void KString::append(KString &str) { _append(str._data, str._size); }

void KString::pop(u32 utf8_count) {
    u32 count = 0;
    for (u32 i = _size - 1; i >= 0 && count < utf8_count; i--) {
        _size--;
        /* pure ascii */
        if ((_data[i] & UTF8_ONE_CODEPOINT_MASK) == _data[i]) {
            count++;
            continue;
        }
        /* utf8 codepoints */
        while ((_data[i] & UTF8_CODEPOINT_MASK) == _data[i]) {
            i--;
            _size--;
        }
        count++;
    }
    _data[_size] = '\0';
}

void KString::pop_raw(u32 count) {
    if (count >= _size) {
        count = _size - 1;
    }
    _size -= count;
    _data[_size] = '\0';
}

KString KString::clone() const {
    KString s;
    s._data = (char *)malloc(this->_size + 1);
    s._size = this->_size;
    s._cap = this->_size;
    memcpy(s._data, _data, _size + 1);
    return s;
}

KString &KString::replace(KStr from, KStr to) {
    if (from == to) return *this;
    KStr s = to_str();

    /* special case if length are equal*/
    if (from.length() == to.length()) {
        std::vector<u32> finds = s.find_all(from);
        for (u32 i : finds) {
            for (u32 j = 0; j < to.length(); j++) {
                _data[i + j] = to.data()[j];
            }
        }
        return *this;
    }

    std::vector<KStr> finds = s.split_n(from, UINT_MAX);
    if (finds.empty()) {
        return *this;
    }
    u32 total_length = 0;
    for (KStr s : finds) {
        total_length += s.length();
    }
    total_length += to.length() * (finds.size());

    /* if capacity is not enough, we reallocate */
    void *original_data = nullptr;
    if (total_length + 1 > _cap) {
        original_data = _data;
        _data = (char *)malloc(total_length + 1);
        _cap = total_length + 1;
    }

    for (u32 i = 0; i < finds.size() - 1; i++) {
        append(finds[i]);
        append(to);
    }
    append(finds.back());
    if (original_data) {
        free(original_data);
    }
    return *this;
}

KString &KString::to_lower() {
    for (u32 i = 0; i < _size; i++) {
        /* we assume all codepoints are valid*/
        u32 codepoint_size = _code_point_length(_data[i]);
        if (codepoint_size == 1) {
            _data[i] |= 0x20;
            continue;
        }
        /* don't handle utf8 */
        i += codepoint_size - 1;
    }
    return *this;
}
KString &KString::to_upper() {
    for (u32 i = 0; i < _size; i++) {
        /* we assume all codepoints are valid*/
        u32 codepoint_size = _code_point_length(_data[i]);
        if (codepoint_size == 1) {
            _data[i] &= ~0x20;
            continue;
        }
        /* don't handle utf8 */
        i += codepoint_size - 1;
    }
    return *this;
}

KStr::KStr(const KString &str) : _data(str.data()), _length(str.size()) {}

KStr::KStr(const KString &str, u32 length) : _data(str.data()) {
    _length = length;
    if (_length > str.size()) {
        _length = str.size();
    }
}

KStr::KStr(const std::string &str) : _data(str.data()), _length(str.size()) {}

KStr::KStr(const Path &path) : KStr(path.to_str()) {}
KString KStr::string() const {
    KString s(_length + 1);
    s.append(*this);
    return s;
}

bool KStr::operator==(const KStr &str) const {
    if (_length != str._length) return false;
    if (_data == str._data) return true;
    for (u32 i = 0; i < _length; i++) {
        if (_data[i] != str._data[i]) return false;
    }
    return true;
}

bool KStr::operator<(const KStr &str) const {
    if (_data == str._data) {
        return _length < str._length;
    }
    for (u32 i = 0; i <= _length && i <= str._length; i++) {
        u8 c1 = this->_data[i];
        u8 c2 = str._data[i];
        if (c1 == c2) {
            continue;
        }
        return c1 < c2;
    }
    return _length < str._length;
}

char KStr::operator[](u32 idx) const { return _data[idx]; }

bool KStr::start_with(const KStr &str) const {
    if (_length < str._length) return false;
    for (u32 i = 0; i < str._length; i++) {
        if (_data[i] != str._data[i]) return false;
    }
    return true;
}
bool KStr::end_with(const KStr &str) const {
    if (_length < str._length) return false;
    for (u32 i = 0; i < str._length; i++) {
        if (_data[_length - 1 - i] != str._data[str._length - 1 - i])
            return false;
    }
    return true;
}
bool KStr::contains(const KStr &str) const { return find_first(str) != -1; }

/* for utf8, the patter need to be unsigned */
static void _critical_factorization(const u8 *pattern, u32 plen,
                                    u32 *suffix_index, u32 *period) {
    /* current max suffix */
    u32 i = -1;
    /* possible max suffix */
    u32 j = 0;
    /* offset of current period*/
    u32 k = 1;
    /* period */
    u32 p = 1;
    if (plen == 1) {
        *suffix_index = 0;
        *period = 1;
        return;
    }

    /* find maximum suffix with lexical order */
    while (j + k < plen) {
        u8 a = pattern[i + k];
        u8 b = pattern[j + k];
        if (a == b) {
            /* current suffix is equal to candicate */
            if (k == p) {
                j += k;
                k = 1;
            } else {
                k++;
            }
        } else if (a < b) {
            /* current suffix is smaller than candicate */
            /* set maximum to candicate and reset period */
            i = j++;
            k = p = 1;
        } else {
            /* current suffix is bigger than candicate */
            /* step k and update period */
            j += k;
            k = 1;
            p = j - i;
        }
    }
    u32 max_suffix_index = i;
    u32 max_p = p;

    i = -1;
    j = 0;
    k = 1;
    p = 1;

    /* find maximum suffix with reverse lexical order */
    while (j + k < plen) {
        u8 a = pattern[i + k];
        u8 b = pattern[j + k];
        if (a == b) {
            if (k == p) {
                j += k;
                k = 1;
            } else {
                k++;
            }
        } else if (a > b) {
            i = j++;
            k = p = 1;
        } else {
            j += k;
            k = 1;
            p = j - i;
        }
    }
    if (i + 1 > max_suffix_index + 1) {
        *suffix_index = i + 1;
        *period = p;
    } else {
        *suffix_index = max_suffix_index + 1;
        *period = max_p;
    }
}

template <bool reverse>
inline static i32 _search(const u8 *haystack, u32 hlen, const u8 *pattern,
                          u32 plen) {
    if (hlen < plen) return -1;

    u32 p, split;
    _critical_factorization(pattern, plen, &split, &p);

    u32 mem = 0;
    i32 i = reverse ? hlen - plen : 0;

    while (reverse ? i >= 0 : i <= hlen - plen) {
        u32 k;
        /* match right side of pattern */
        for (k = std::max(0u, split); k < plen && haystack[i + k] == pattern[k];
             k++);
        /* match failed, we could step */
        /* the size of char matches. */
        if (k < plen) {
            if constexpr (reverse)
                i -= k - split + 1;
            else
                i += k - split + 1;
            mem = 0;
            continue;
        }
        /* match left side of pattern */
        for (k = split; k > mem && haystack[i + k - 1] == pattern[k - 1]; k--);
        /* match success */
        if (k <= mem) return i;
        if (reverse)
            i -= p;
        else
            i += p;
        mem = plen - p;
    }
    return -1;
}

template <typename Fn>
inline static void _search_n(const u8 *haystack, u32 hlen, const u8 *pattern,
                             u32 plen, u32 n, Fn func) {
    if (hlen < plen) return;
    u32 p, split;
    _critical_factorization(pattern, plen, &split, &p);

    u32 mem = 0;
    u32 i = 0;
    u32 count = 0;

    while (count < n && i <= hlen - plen) {
        u32 k;
        for (k = std::max(0u, split); k < plen && haystack[i + k] == pattern[k];
             k++);
        if (k < plen) {
            i += k - split + 1;
            mem = 0;
            continue;
        }

        for (k = split; k > mem && haystack[i + k - 1] == pattern[k - 1]; k--);
        if (k <= mem) {
            func(i);
            count++;
        }
        i += p;
        mem = plen - p;
    }
}

/* return -1 if not found */
i32 KStr::find_first(const KStr &pattern) const {
    return _search<false>((u8 *)_data, _length, (u8 *)pattern._data,
                          pattern._length);
}
i32 KStr::find_last(const KStr &pattern) const {
    return _search<true>((u8 *)_data, _length, (u8 *)pattern._data,
                         pattern._length);
}
std::vector<u32> KStr::find_all(const KStr &pattern) const {
    std::vector<u32> indices;
    _search_n((u8 *)_data, _length, (u8 *)pattern._data, pattern._length,
              UINT_MAX, [&](u32 index) { indices.push_back(index); });
    return indices;
}
std::vector<KStr> KStr::split(const KStr &pattern) const {
    return split_n(pattern, UINT_MAX);
}

std::vector<KStr> KStr::split_n(const KStr &pattern, u32 n) const {
    std::vector<KStr> splits;
    u32 last_split = 0;
    _search_n((u8 *)_data, _length, (u8 *)pattern._data, pattern._length, n,
              [&](u32 index) {
                  splits.push_back(KStr((char *)(u64)(_data + last_split),
                                        index - last_split));
                  last_split += index + pattern.length() - last_split;
              });
    splits.push_back(
        KStr((char *)(u64)(_data + last_split), _length - last_split));
    return splits;
}

std::pair<KStr, KStr> KStr::split_at(u32 i) const {
    std::pair<KStr, KStr> pair;
    if (i >= _length) {
        i = _length - 1;
    }
    pair.first = KStr(_data, i);
    pair.second = KStr((char *)(u64)(_data + i), _length - i);
    return pair;
}

KStr KStr::ltrim() const {
    KStr trimmed;
    u32 i = 0;
    while (i < _length) {
        u8 c = _data[i];
        if (!(c == ' ' || c == '\t' || c == '\n')) {
            break;
        }
        i++;
    };
    trimmed._data = (char *)(u64)_data + i;
    trimmed._length = _length - i;
    return trimmed;
}
KStr KStr::rtrim() const {
    KStr trimmed;
    u32 i = 0;
    while (i < _length) {
        u8 c = _data[_length - i - 1];
        if (!(c == ' ' || c == '\t' || c == '\n')) {
            break;
        }
        i++;
    };
    trimmed._data = (char *)(u64)_data;
    trimmed._length = _length - i;
    return trimmed;
}
KStr KStr::trim() const { return ltrim().rtrim(); }

KString KStr::replace(const KStr &pattern, const KStr &to) const {
    return replace_n(pattern, to, UINT_MAX);
}
KString KStr::replace_n(const KStr &pattern, const KStr &to, u32 n) const {
    std::vector<KStr> finds = split_n(pattern, n);
    if (finds.empty()) {
        return this->string();
    }
    u32 total_length = 0;
    for (KStr s : finds) {
        total_length += s._length;
    }
    total_length += to._length * (finds.size());
    KString str(total_length + 1);
    for (u32 i = 0; i < finds.size() - 1; i++) {
        str.append(finds[i]);
        str.append(to);
    }
    str.append(finds.back());
    return str;
}

}  // namespace Seed