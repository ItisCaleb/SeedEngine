#ifndef _SEED_ENUMS_H_
#define _SEED_ENUMS_H_
#include <string_view>
#include <array>

namespace Seed {

template <auto Enum>
constexpr auto get_enum_name() {
#if defined(__clang__)
    constexpr auto prefix = std::string_view{"[Enum = "};
    constexpr auto suffix = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
    constexpr auto prefix = std::string_view{"with Enum = "};
    constexpr auto suffix = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#else
#error Unsupported compiler
#endif

    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);

    static_assert(start < end);

    constexpr auto name = function.substr(start, (end - start));
    return name;
}

template <std::size_t... Idxs>
constexpr auto _substring_as_array(std::string_view str,
                                   std::index_sequence<Idxs...>) {
    return std::array{str[Idxs]...};
}

/* a trick to prevent raw Function name in binary */
template <auto Enum>
constexpr auto enum_name_array() {
    constexpr auto name = get_enum_name<Enum>();
    return _substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <auto T>
struct _enum_name_holder {
        static inline constexpr auto value = enum_name_array<T>();
};

template <auto Enum>
constexpr auto enum_name() {
    constexpr auto &value = _enum_name_holder<Enum>::value;
    return std::string_view{value.data(), value.size()};
}

template <auto Enum>
constexpr auto get_enum_member_name() {
    constexpr auto name = enum_name<Enum>();
    constexpr auto start = name.rfind("::") + 2;
    static_assert(start < name.size());

    return name.substr(start);
}

template <auto Enum>
constexpr bool _is_enum_valid() {
    constexpr auto name = enum_name<Enum>();
    constexpr auto invalid = "(";
    return name.find(invalid) == std::string_view::npos;
}

template <typename T, std::size_t... I>
constexpr auto _get_enum_candidates(std::index_sequence<I...>) {
    return std::array<bool, 256>{
        _is_enum_valid<static_cast<T>(static_cast<int>(I))>()...};
}

template <typename T>
constexpr std::size_t _get_enum_valid_count() {
    constexpr auto candidates =
        _get_enum_candidates<T>(std::make_index_sequence<256>{});
    std::size_t cnt = 0;
    for (bool b : candidates)
        if (b) cnt++;
    return cnt;
}

template <typename T>
constexpr auto _get_enums() {
    constexpr auto candidates =
        _get_enum_candidates<T>(std::make_index_sequence<256>{});
    std::array<T, 256> enums{};
    int cnt = 0;
    for (int i = 0; i < 256; i++) {
        if (candidates[i]) {
            enums[cnt] = (T)i;
            cnt++;
        }
    }
    return enums;
}

template <typename T>
constexpr auto enum_array() {
    constexpr int enum_count = _get_enum_valid_count<T>();
    constexpr auto enums = _get_enums<T>();
    std::array<T, enum_count> enum_arr;
    for (int i = 0; i < enum_count; i++) {
        enum_arr[i] = enums[i];
    }
    return enum_arr;
}

template <typename T, std::size_t... I>
constexpr auto _get_enum_member_names(std::index_sequence<I...>) {
    constexpr auto candidates =
        _get_enum_candidates<T>(std::make_index_sequence<256>{});
    std::array<std::string_view, 256> names{};
    int j = 0;
    (..., (candidates[I]
               ? (names[j++] = get_enum_member_name<static_cast<T>(I)>(), 0)
               : 0));
    return names;
}

template <typename T>
constexpr auto enum_member_names() {
    constexpr int enum_count = _get_enum_valid_count<T>();
    constexpr auto names =
        _get_enum_member_names<T>(std::make_index_sequence<256>{});
    std::array<std::string_view, enum_count> enum_names;
    for (int i = 0; i < enum_count; i++) {
        enum_names[i] = names[i];
    }
    return enum_names;
}
template <typename T>
constexpr std::string_view enum_to_string(T e) {
    auto enums = enum_array<T>();
    auto names = enum_member_names<T>();
    for (int i = 0; i < enums.size(); i++) {
        if (enums[i] == e) return names[i];
    }

    return "";
}

template <typename T>
constexpr T string_to_enum(std::string_view s) {
    auto enums = enum_array<T>();
    auto names = enum_member_names<T>();
    for (int i = 0; i < enums.size(); i++) {
        if (names[i] == s) return enums[i];
    }
    return enums[enums.size() - 1];
}

}  // namespace Seed

#endif
