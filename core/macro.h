#ifndef SEED_MACRO_H_
#define SEED_MACRO_H_
#include <spdlog/spdlog.h>
#include <stdexcept>

#define EXPECT_NOT_NULL_RET(val)                    \
    if ((val) == nullptr) {                         \
        SPDLOG_ERROR("Variable " #val " is null."); \
        return;                                     \
    } else                                          \
        (void(0))

#define EXPECT_NOT_NULL_RET_MSG(val, msg)               \
    if ((val) == nullptr) {                             \
        SPDLOG_ERROR("Variable " #val " is null." msg); \
        return;                                         \
    } else                                              \
        (void(0))

#define EXPECT_INDEX_INBOUND(num, size)                      \
    if ((num) >= (size) || (num) < 0) {                      \
        SPDLOG_ERROR("{} is out of " #size " bound .", num); \
        return;                                              \
    } else                                                   \
        (void(0))

#define EXPECT_INDEX_INBOUND_THROW(num, size)                \
    if ((num) >= (size) || (num) < 0) {                      \
        SPDLOG_ERROR("{} is out of " #size " bound .", num); \
        throw std::out_of_range("");                         \
    } else                                                   \
        (void(0))

#define EXPECT_INDEX_INBOUND_RET(num, size, ret)             \
    if ((num) >= (size) || (num) < 0) {                      \
        SPDLOG_ERROR("{} is out of " #size " bound .", num); \
        return ret;                                          \
    } else                                                   \
        (void(0))

#endif