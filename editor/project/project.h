#ifndef _SEED_PROJECT_H_
#define _SEED_PROJECT_H_
#include <string>
#include <map>
#include "core/world.h"

namespace Seed {
class EditorGUI;
class Project {
        friend EditorGUI;

    private:
        std::string name;
        std::string path;

        void save();

    public:
        static Project *load(const std::string &path);
};
}  // namespace Seed

#endif