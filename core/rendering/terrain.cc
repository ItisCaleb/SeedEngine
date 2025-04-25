#include "terrain.h"

namespace Seed {
Terrain::Terrain(u32 width, u32 depth, Ref<Texture> height_map):width(width), depth(depth), height_map(height_map) {}

Terrain::~Terrain(){
}
}  // namespace Seed