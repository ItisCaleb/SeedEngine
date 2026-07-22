#include "profiler.h"
#include "core/system.h"
#include <algorithm>
#include <chrono>
#include <utility>
#include "core/container/kstring.h"

namespace Seed {
ProfileScope::ProfileScope(KStr name) : name(name) {
    begin = std::chrono::steady_clock::now();
    System::gProfiler->scope_stack.push_back(name);
}
ProfileScope::~ProfileScope() {
    auto end = std::chrono::steady_clock::now();
    auto cpu_time =
        std::chrono::duration_cast<std::chrono::microseconds>(end - begin)
            .count();

    System::gProfiler->scope_stack.pop_back();
    System::gProfiler->record_scope(name, cpu_time);
}
Profiler::Profiler() {}

void Profiler::record_scope(KStr name, u64 time) {
    KString full_name;
    for (auto &v : scope_stack) {
        full_name.append(v);
        full_name.push('/');
    }
    full_name.append(name);
    recorded_scope.push_back(
        RecordScope{.name = std::move(full_name), .cpu_time = time});
}

void Profiler::clear_records() {
    last_recorded_scope.resize(recorded_scope.size());
    std::copy(recorded_scope.begin(), recorded_scope.end(),
              last_recorded_scope.begin());
    recorded_scope.clear();
}
}  // namespace Seed
