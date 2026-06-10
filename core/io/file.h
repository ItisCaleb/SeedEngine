#ifndef _SEED_FILE_H_
#define _SEED_FILE_H_
#include "core/container/kstring.h"
#include "core/ref.h"
#include "core/types.h"
#include "path.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
namespace Seed {

#ifndef SPLITOR
#ifdef _WIN32
#define SPLITOR "\\"
#else
#define SPLITOR "/"
#endif
#endif

class File : public RefCounted {
    private:
        FILE *file;
        Path path;
        /* lazy evaluation full path */
        mutable Path full_path;
        u64 file_size;
        u64 read_cnt;
        u64 write_cnt;

    public:
        static Ref<File> open(KStr path, const char *mode = "rb");
        static bool exists(const Path &path);
        static bool remove(const Path &path);

        std::string read_str(size_t size = SIZE_MAX);

        void read(void *data, size_t size);

        template <typename T>
        void read(T *data) {
            size_t size = sizeof(T);
            read(data, size);
        }

        nlohmann::json read_json();

        template <typename T>
        void read_vector(std::vector<T> &vec, u32 cnt) {
            size_t ele_size = sizeof(T);
            vec.resize(cnt);
            read(vec.data(), cnt * ele_size);
        }

        size_t write(void *data, size_t size);

        template <typename T>
        size_t write(T *data) {
            size_t size = sizeof(T);
            return write(data, size);
        }

        template <typename T>
        size_t write(std::vector<T> &vec) {
            size_t size = vec.size() * sizeof(T);
            return write(vec.data(), size);
        }

        size_t write_str(const std::string &str) const;

        void copy_to(const Path &path) const;

        const Path &get_path() const { return this->path; }
        const Path &get_fullpath() const {
            if (full_path.is_empty()) {
                full_path = this->path;
                full_path.absolute();
            }
            return this->full_path;
        }
        const KStr get_filename() const { return this->path.filename(); }

        const KStr get_filename_without_ext() const {
            return this->path.filename_without_ext();
        }

        const KStr get_directory() { return get_fullpath().directory(); }

        ~File() {
            if (file) {
                fclose(file);
            }
        }
};

}  // namespace Seed
#endif