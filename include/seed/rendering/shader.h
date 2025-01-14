#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_
#include <seed/types.h>
#include <seed/ref.h>

namespace Seed{
    class Shader : public RefCounted{
    private:
        u32 id;
    public:
        Shader(u32 id);
        void use();
        u32 get_id();
    };
    
    
}

#endif