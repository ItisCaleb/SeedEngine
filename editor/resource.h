#ifndef _EDITOR_RESOURCE_H_
#define _EDITOR_RESOURCE_H_
#include "core/rendering/mesh.h"
#include <string>

Seed::Ref<Seed::Mesh> parseMesh(const std::string &path);

#endif