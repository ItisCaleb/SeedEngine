#ifndef _SEED_PATH_H_
#define _SEED_PATH_H_

#include <vector>
#include "core/container/kstring.h"
#include "core/os.h"
namespace Seed {
class Path {
    protected:
        KString path;
        KStr root;
        const static constexpr KStr win_spliiter = "\\";
        const static constexpr KStr win_dds = "..\\";
        const static constexpr KStr win_ds = ".\\";
        const static constexpr KStr unix_splitter = "/";
        const static constexpr KStr unix_dds = "../";
        const static constexpr KStr unix_ds = "./";
        constexpr KStr get_splitter() const {
            if (OS::get_os() == OSType::WINDOWS) {
                return win_spliiter;
            } else {
                return unix_splitter;
            }
        }

        constexpr KStr get_dds() const {
            if (OS::get_os() == OSType::WINDOWS) {
                return win_dds;
            } else {
                return unix_dds;
            }
        }

        constexpr KStr get_ds() const {
            if (OS::get_os() == OSType::WINDOWS) {
                return win_ds;
            } else {
                return unix_ds;
            }
        }

    public:
        Path() = default;
        Path(const KStr &str);
        Path(const char *str) : Path(KStr(str)) {}
        Path(const Path &path) { operator=(path); }
        const char *data() const { return path.data(); }
        Path &operator=(const KStr &str);
        Path &operator=(const Path &path);
        bool operator==(const Path &path) const;
        bool operator!=(const Path &path) const { return !operator==(path); }
        bool operator<(const Path &path) const {
            return to_str() < path.to_str();
        };

        void normalize();
        void push(const KStr &segment);
        void pop();
        Path append(const KStr &segment) const;
        bool is_empty() { return path.is_empty(); }

        /* Warning: Do not modify Path instance after this call! */
        KStr extension() const;

        /* Warning: Do not modify Path instance after this call!*/
        KStr filename() const;

        /* Do not use the return KStr as null terminated string! */
        KStr filename_without_ext() const;

        /* Do not use the return KStr as null terminated string! */
        KStr directory() const;
        Path parent() const;
        Path replace_extension(KStr str) const;
        KStr to_str() const;
        
        Path relative(const Path &base) const;
        bool is_absolute() const;
        bool absolute();
        bool canonicalize();
        bool is_directory() const;
        bool is_file() const;
};

}  // namespace Seed

template <>
struct std::hash<Seed::Path> {
        size_t operator()(const Seed::Path &p) const noexcept {
            Seed::KStr s = p.to_str();
            return Seed::Hash::hash_from_buffer(s.data(), s.length());
        }
};

template <>
struct fmt::formatter<Seed::Path> : fmt::formatter<fmt::string_view> {
        constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }

        auto format(const Seed::Path &path, format_context &ctx) const {
            // Cast or convert your type to fmt::string_view and delegate
            return formatter<fmt::string_view>::format(
                {(char *)path.data(), path.to_str().length()}, ctx);
        }
};
#endif