#include "path.h"
#include <direct.h>
#include <utility>
#include <vector>
#include "core/container/kstring.h"
#include "core/io/dir.h"
#include "core/types.h"
#include "file.h"
#ifdef _WIN32
#include <windows.h>
#endif

namespace Seed {
Path::Path(const KStr &str) : path(64) { operator=(str); }

Path &Path::operator=(const KStr &str) {
    KStr _path = str.trim();
    if (str.length() == 0) {
        path.append(".");
        root = KStr(path, 1);
        return *this;
    }
    path.append(_path);
    path.replace(unix_splitter, get_splitter());
    path.replace(win_spliiter, get_splitter());

    u8 first_char = _path.data()[0];
    if ((first_char >= 'a' && first_char <= 'z') ||
        (first_char >= 'A' && first_char <= 'Z')) {
        /* start with Drive, e.g C:// */
        if (_path.length() >= 3 && _path.data()[1] == ':' &&
            (_path.data()[2] == get_splitter().data()[0])) {
            root = KStr(path, 3);
            return *this;
        } else {
            root = KStr(path, 0);
            return *this;
        }
    }

    if (_path == ".." || _path.start_with(get_dds())) {
        root = KStr(path, 2);
    } else if (_path == "." || _path.start_with(get_ds())) {
        root = KStr(path, 1);
    }

    else if (_path.start_with("/")) {
        root = KStr(path, 1);
    }
    return *this;
}

Path &Path::operator=(const Path &path) {
    this->path = path.path;
    root = KStr(this->path, path.root.length());
    return *this;
}

bool Path::operator==(const Path &path) const {
    return to_str() == path.to_str();
}

void Path::normalize() {
    KStr _path = path.to_str();
    if (_path.is_empty()) return;

    bool has_drive = root.length() == 3;
    bool is_absolute = root == get_splitter() || has_drive;

    std::vector<KStr> segments = _path.split(get_splitter());
    std::vector<KStr> stack;
    stack.reserve(segments.size());

    u32 is_normalized = true;
    for (const KStr &seg : segments) {
        if (seg.length() == 0 || seg == ".") {
            is_normalized = false;
            continue;
        } else if (seg == "..") {
            if (!stack.empty() && stack.back() != "..") {
                stack.pop_back();
                is_normalized = false;
            } else if (!is_absolute) {
                stack.push_back(seg);
            }
        } else {
            stack.push_back(seg);
        }
    }
    if (is_normalized) {
        return;
    }

    /* Since  */
    if (is_absolute) {
        path.resize(root.length());
    } else {
        path.clear();
    }
    for (i32 i = 0; i < (i32)stack.size(); i++) {
        /* skip windows drive */
        if (i == 0 && has_drive) continue;
        if (i > 0 && !(i == 1 && has_drive)) path.append(get_splitter());
        path.append(stack[i]);
    }

    if (path.is_empty()) {
        path.append(".");
        root = KStr(path, 1);
    }
}
void Path::push(KStr segment) {
    if (!path.to_str().is_empty() && !path.to_str().end_with(get_splitter())) {
        path.append(get_splitter());
    }
    path.append(segment);
}
void Path::pop() {
    KStr _path = path.to_str();
    if (_path == root) {
        return;
    }
    i32 i = _path.find_last(get_splitter());
    if (i > 0) {
        path.pop_raw(_path.length() - i);
    } else if (i == 0) {
        path.pop_raw(_path.length() - i - 1);
    }
}

Path Path::append(KStr segment) const {
    Path new_path = *this;
    new_path.push(segment);
    return new_path;
}

KStr Path::extension() const {
    KStr _path = path.to_str();
    i32 i = _path.find_last(".");
    if (i == -1) {
        return KStr((char *)(u64)_path.data() + _path.length(), 0);
    }
    return _path.split_at(i).second;
}

KStr Path::filename() const {
    KStr _path = path.to_str();
    if (_path == root) {
        return KStr((char *)(u64)_path.data() + _path.length(), 0);
    }
    i32 i = _path.find_last(get_splitter());
    /* doesn't find splitter */
    if (i == -1) {
        return _path;
    }
    /* splitter is at last */
    if (i == _path.length() - 1) {
        return KStr((char *)(u64)_path.data() + _path.length(), 0);
    }
    /* do not contain splitter */
    i++;

    return _path.split_at(i).second;
}

KStr Path::filename_without_ext() const {
    KStr filename = this->filename();
    i32 i = filename.find_last(".");
    if (i == -1) {
        return filename;
    }
    return filename.split_at(i).first;
}

KStr Path::directory() const {
    KStr _path = path.to_str();
    if (_path == root) {
        return _path;
    }
    /* single segment*/
    i32 back = _path.find_last(get_splitter());
    if (back == -1) {
        return _path;
    }
    if (is_file()) {
        _path = _path.split_at(back).first;
        i32 front = _path.find_last(get_splitter());
        /* we find root*/
        if (front <= root.length()) {
            return root;
        }
        _path = _path.split_at(front + 1).second;
    } else {
        return _path.split_at(back + 1).second;
    }

    return _path;
}

Path Path::parent() const {
    Path parent = *this;
    parent.pop();
    return parent;
}

Path Path::replace_extension(KStr str) const {
    Path new_path = *this;
    KStr _path = new_path.to_str();
    i32 i = _path.find_last(".");
    if (i != -1) {
        new_path.path.pop_raw(_path.length() - i);
        new_path.path.append(".");
    }
    new_path.path.append(str);
    return new_path;
}

KStr Path::to_str() const { return KStr(path); }

Path Path::relative(const Path &base) const {
    if (base.root.is_empty()) {
        return *this;
    }

    if (root.length() == 3 && base.root.length() == 3) {
        /* Different drive（Windows）*/
        if (root != base.root) {
            return *this;
        }
    } else {
        /* path is already relative */
        if (root.is_empty() || !(root == get_splitter())) {
            return *this;
        }
    }

    Path result;
    KStr _path = path.to_str();
    KStr _base = base.path.to_str();

    std::vector<KStr> path_segs = _path.split(get_splitter());
    std::vector<KStr> base_segs = _base.split(get_splitter());

    /* find common */
    u64 common = 0;
    u64 min_len = min(path_segs.size(), base_segs.size());
    while (common < min_len && path_segs[common] == base_segs[common]) {
        common++;
    }

    /* every left segment is ".." */
    for (u64 i = common; i < base_segs.size(); i++) {
        if (base_segs[i].is_empty()) continue;
        result.push(get_dds());  // ".."
    }

    /* cosume left over */
    for (u64 i = common; i < path_segs.size(); i++) {
        if (path_segs[i].is_empty()) continue;
        result.push(path_segs[i]);
    }

    /* if path is same */
    if (result.path.is_empty()) {
        result.path.append(".");
        result.root = KStr(result.path, 1);
    }

    return result;
}

bool Path::is_absolute() const {
    bool is_absolute = root == get_splitter() || root.length() == 3;
    return is_absolute;
}

bool Path::absolute() {
    /* already absoute */
    bool is_absolute = root == get_splitter() || root.length() == 3;
    if (is_absolute) {
        normalize();
        return true;
    }
    /*  get cwd */
    char cwd_buf[4096];
#ifdef _WIN32
    if (!_getcwd(cwd_buf, sizeof(cwd_buf))) return false;
#else
    if (!getcwd(cwd_buf, sizeof(cwd_buf))) return false;
#endif

    KString new_path(64);
    new_path.append(KStr(cwd_buf));
    new_path.append(get_splitter());
    new_path.append(path.to_str());

    path = std::move(new_path);

#ifdef _WIN32
    root = KStr(path, 3);  // "C:/"
#else
    root = KStr(path, 1);  // "/"
#endif

    normalize();
    return true;
}

bool Path::canonicalize() {
#ifdef _WIN32
    char buf[4096];
    HANDLE h = CreateFileA(
        path.data(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;

    DWORD len =
        GetFinalPathNameByHandleA(h, buf, sizeof(buf), FILE_NAME_NORMALIZED);
    CloseHandle(h);
    if (len == 0 || len >= sizeof(buf)) return false;

    path.clear();
    path.append(KStr(buf, len));
    root = KStr(path, 3);
#else
    char buf[PATH_MAX];
    if (!realpath(path.data(), buf)) return false;

    path.clear();
    path.append(KStr(buf));
    root = KStr(path, 1);
#endif
    return true;
}

bool Path::is_directory() const { return Dir::exists(*this); }
bool Path::is_file() const { return File::exists(*this); }

}  // namespace Seed
