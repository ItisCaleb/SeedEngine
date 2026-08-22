#ifndef _SEED_SOURCE_LOCATION_H_
#define _SEED_SOURCE_LOCATION_H_
#include "core/container/kstring.h"
#include "core/types.h"

namespace Seed {
struct SourceLocation {
        KStr file;
        KStr function;
        u32 line;

        static constexpr SourceLocation current(
            const char *file = __builtin_FILE(),
            const char *function = __builtin_FUNCTION(),
            u32 line = __builtin_LINE()) {
            return SourceLocation{file, function, line};
        }
};
}  // namespace Seed

#endif