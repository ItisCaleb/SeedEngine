#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include "core/io/path.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"
#include "core/rendering/shader_layout.h"

namespace Seed {
class Shader : public Resource {
    private:
        ShaderHandle handle;
        ShaderLayout layout;

    public:
        Shader(const Path &path, const std::string &code);
        ShaderHandle get_handle() { return handle; }
        ShaderLayout &get_layout() { return layout; }
        ~Shader() { RHI::dealloc(handle); }
};
}  // namespace Seed

#endif