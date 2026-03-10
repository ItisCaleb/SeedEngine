#ifndef _SEED_FILE_H_
#define _SEED_FILE_H_
#include "core/ref.h"
#include "core/types.h"
#include <stdio.h>
#include <vector>
#include <string>
#include <filesystem>
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
        std::string path;
        std::string full_path;
        u64 file_size;
        u64 read_cnt;
        u64 write_cnt;

    public:
        static Ref<File> open(const std::string &path, const char *mode = "rb");
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

        void copy_to(const std::string &path) const;

        const std::string &get_path() const { return this->path; }
        const std::string &get_fullpath() const { return this->full_path; }
        const std::string get_filename() const {
            std::string_view view(this->full_path);
            view = view.substr(view.find_last_of(SPLITOR) + 1);
            return std::string(view);
        }

        const std::string get_filename_without_ext() const {
            std::string_view view(this->full_path);
            view = view.substr(view.find_last_of(SPLITOR));
            view = view.substr(0, view.find_last_of("."));
            return std::string(view);
        }

        const std::string get_directory() {
            std::string_view view(this->full_path);
            auto f = view.find_last_of(SPLITOR);
            view = view.substr(0, f);
            return std::string(view);
        }

        ~File() {
            if (file) {
                fclose(file);
            }
        }
};

}  // namespace Seed
#endif