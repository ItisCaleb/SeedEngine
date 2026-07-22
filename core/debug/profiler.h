#ifndef _SEED_PROFILER_H_
#define _SEED_PROFILER_H_

#include "core/system.h"
#include <vector>
#include "core/container/kstring.h"
#include "core/types.h"
#include <chrono>
#include "core/macro.h"
namespace Seed {
class Profiler;
class ProfileScope {
        friend Profiler;

    public:
        KString name;
        std::chrono::time_point<std::chrono::steady_clock> begin;
        ProfileScope() = default;

        ProfileScope(KStr name);
        ~ProfileScope();
};

struct RecordScope {
        KString name;
        u64 cpu_time;
};

class Profiler {
        friend ProfileScope;

    public:
        std::vector<KStr> scope_stack;
        std::vector<RecordScope> recorded_scope;
        std::vector<RecordScope> last_recorded_scope;
        void record_scope(KStr name, u64 time);
        std::vector<RecordScope> &get_recorded() { return last_recorded_scope; }
        void clear_records();
        Profiler();
};
#define PROFILE_SCOPE(name) CONCAT(ProfileScope _prof_, __LINE__)(name)

};  // namespace Seed

#endif
