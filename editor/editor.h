#ifndef _SEED_EDITOR_H_
#define _SEED_EDITOR_H_
#include <string>
#include "project/project.h"

namespace Seed {
class EditorGUI;
class Editor {
        friend EditorGUI;
        inline static Editor *instance = nullptr;
        Project *current_project = nullptr;
    public:
        Editor();
        ~Editor() = default;
};
}  // namespace Seed

#endif