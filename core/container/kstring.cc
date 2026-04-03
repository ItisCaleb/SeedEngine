#include "kstring.h"
#include <fmt/base.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "core/types.h"

namespace Seed {

KString::KString(u32 cap) : _cap(cap), _size(1) {
    if (cap == 0) {
        _size = 0;
        return;
    }
    _data = (u8 *)malloc(_cap);
}
KString::KString(char *str) : KString((const char *)str) {}
KString::KString(const char *str) {
    _size = strlen(str) + 1;
    _cap = _size;
    _data = (u8 *)malloc(_cap);
    memcpy(_data, str, _cap);
}

KString::KString(const std::string &str) {
    _size = str.length() + 1;
    _cap = _size + 1;
    _data = (u8 *)malloc(_cap);
    memcpy(_data, str.data(), _cap);
}

KStr KString::to_str() { return KStr(*this); }

void KString::operator=(KString &str) {
    _size = str._size;
    _cap = str._cap;
    _data = (u8 *)malloc(_cap);
    memcpy(_data, str._data, _cap);
}

KString::KString(KString &&str) {
    _data = str._data;
    _size = str._size;
    _cap = str._cap;
}

KString::~KString() {
    if (_data) {
        free(_data);
    }
}

u32 KString::length() {
    u32 length = 0;
    /* assert string is valid utf8 */
    for (u32 i = 0; i < _size; i++) {
        length++;
        i += _code_point_length(_data[i]);
    }
    return length;
}

void KString::_append(const u8 *c, u32 size) {
    if (_data == nullptr) {
        _size = 1;
        _cap = std::max(size, 16u);
        _data = (u8 *)malloc(_cap);
    }
    if (_cap < _size + size) {
        u32 new_size = (_cap + size) << 1;
        void *tmp = realloc(_data, new_size);
        if (tmp == nullptr) {
            throw std::runtime_error("KString reallocation error.");
        }
        _cap = new_size;
        _data = (u8 *)tmp;
    }
    memcpy(&_data[_size - 1], c, size);
    _size += size;
    _data[_size - 1] = '\0';
}

void KString::push(const char c) { _append((const u8 *)&c, 1); }
void KString::push(const KChar &c) {
    _append(c.b, c.size());
    return;
}

void KString::append(const KStr &str) { _append(str.data(), str.length()); }
void KString::append(KString &str) { _append(str._data, str._size - 1); }

KString KString::clone() {
    KString s;
    s._data = (u8 *)malloc(this->_size);
    s._size = this->_size;
    return s;
}

KStr::KStr(KString &str) : _data(str.data()), _length(str.size() - 1) {}

KString KStr::string() {
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
    u32 i = reverse ? hlen - plen - 1 : 0;

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
        for (k = split - 1; k > mem && haystack[i + k] == pattern[k]; k--);
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
    return _search<false>(_data, _length, pattern._data, pattern._length);
}
i32 KStr::find_last(const KStr &pattern) const {
    return _search<true>(_data, _length, pattern._data, pattern._length);
}
std::vector<u32> KStr::find_all(const KStr &pattern) const {
    std::vector<u32> indices;
    _search_n(_data, _length, pattern._data, pattern._length, UINT_MAX,
              [&](u32 index) { indices.push_back(index); });
    return indices;
}
std::vector<KStr> KStr::split(const KStr &pattern) const {
    return split_n(pattern, UINT_MAX);
}

std::vector<KStr> KStr::split_n(const KStr &pattern, u32 n) const {
    std::vector<KStr> splits;
    u32 last_split = 0;
    _search_n(
        _data, _length, pattern._data, pattern._length, n, [&](u32 index) {
            splits.push_back(
                KStr((u8 *)(u64)(_data + last_split), index - last_split));
            last_split += index + pattern.length() - last_split;
        });
    splits.push_back(
        KStr((u8 *)(u64)(_data + last_split), _length - last_split));
    return splits;
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
    trimmed._data = (u8 *)(u64)_data + i;
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
    trimmed._data = (u8 *)(u64)_data;
    trimmed._length = _length - i;
    return trimmed;
}
KStr KStr::trim() const { return ltrim().rtrim(); }

KString KStr::replace(const KStr &pattern, const KStr &to) {
    return replace_n(pattern, to, UINT_MAX);
}
KString KStr::replace_n(const KStr &pattern, const KStr &to, u32 n) {
    std::vector<KStr> finds = split_n(pattern, n);
    if (finds.empty()) {
        return this->string();
    }
    u32 total_length = 0;
    for (KStr s : finds) {
        total_length += s._length;
    }
    total_length += to._length * (finds.size() - 1);
    KString str(total_length + 1);
    for (u32 i = 0; i < finds.size() - 1; i++) {
        str.append(finds[i]);
        str.append(to);
    }
    str.append(finds.back());
    return str;
}

}  // namespace Seed