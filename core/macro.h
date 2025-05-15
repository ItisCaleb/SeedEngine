#ifndef SEED_MACRO_H_
#define SEED_MACRO_H_
#include <spdlog/spdlog.h>

#define EXPECT_NOT_NULL_RET(val)                                        \
    if (val == nullptr) {                                               \
        SPDLOG_ERROR("Variable " #val " is null.", __FILE__, __LINE__); \
        return;                                                         \
    } else                                                              \
        (void(0))

#define EXPECT_NOT_NULL_RET_MSG(val, msg)                                   \
    if (val == nullptr) {                                                   \
        SPDLOG_ERROR("Variable " #val " is null." msg, __FILE__, __LINE__); \
        return;                                                             \
    } else                                                                  \
        (void(0))

#endif