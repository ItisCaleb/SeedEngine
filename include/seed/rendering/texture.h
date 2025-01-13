#ifndef _SEED_TEXTURE_H_
#define _SEED_TEXTURE_H_
#include <seed/types.h>
#include <seed/ref.h>

namespace Seed {
struct Texture : public RefCounted{
    u32 id;
};
}  // namespace Seed

#endif