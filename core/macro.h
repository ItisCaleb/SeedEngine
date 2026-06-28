#ifndef SEED_MACRO_H_
#define SEED_MACRO_H_
#include <spdlog/spdlog.h>

#define CONCAT_INTERNAL(a, b) a##b

#define CONCAT(a, b) CONCAT_INTERNAL(a, b)

#define SEED_INSTANCE(_class)                                \
private:                                             \
    inline static _class *instance = nullptr; \
public:                                              \
    static _class *get_instance() { return instance; }

#define EXPECT_NOT_NULL_RET(val, ...)                                 \
    if ((val) == nullptr) {                                           \
        SPDLOG_ERROR("{}: Variable " #val " is null.", __FUNCTION__); \
        return __VA_ARGS__;                                           \
    } else                                                            \
        (void(0))

#define EXPECT_NOT_NULL_BREAK(val)                                    \
    if ((val) == nullptr) {                                           \
        SPDLOG_ERROR("{}: Variable " #val " is null.", __FUNCTION__); \
        break;                                                        \
    } else                                                            \
        (void(0))

#define EXPECT_NOT_NULL_RET_MSG(val, msg)                                 \
    if ((val) == nullptr) {                                               \
        SPDLOG_ERROR("{}: Variable " #val " is null." msg, __FUNCTION__); \
        return;                                                           \
    } else                                                                \
        (void(0))

#define EXPECT_INDEX_INBOUND(num, size)                                    \
    if ((num) >= (size) || (num) < 0) {                                    \
        SPDLOG_ERROR("{}: " #num "'{}' is out of range, max size is {} .", \
                     num, size, __FUNCTION__);                             \
        return;                                                            \
    } else                                                                 \
        (void(0))

#define EXPECT_INDEX_INBOUND_THROW(num, size)                              \
    if ((num) >= (size) || (num) < 0) {                                    \
        SPDLOG_ERROR("{}: " #num "'{}' is out of range, max size is {} .", \
                     num, size, __FUNCTION__);                             \
        throw std::out_of_range("");                                       \
    } else                                                                 \
        (void(0))

#define EXPECT_INDEX_INBOUND_RET(num, size, ret)                         \
    if ((num) >= (size) || (num) < 0) {                                  \
        SPDLOG_ERROR("{}: {} is out of range, size is " #size " .", num, \
                     __FUNCTION__);                                      \
        return ret;                                                      \
    } else                                                               \
        (void(0))

#endif

#define SEED_WARN(msg, ...) spdlog::warn("{}: " msg, __FUNCTION__, __VA_ARGS__)
