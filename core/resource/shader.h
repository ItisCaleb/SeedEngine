#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include "core/handle.h"
#include "core/io/path.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"
#include "core/rendering/shader_layout.h"

namespace Seed {
class Shader : public Resource {
    private:
        ShaderHandle handle;
        ConstantHandle param_handle = NULL_HANDLE;
        ShaderLayout layout;

    public:
        Shader(const Path &path, const std::string &code);
        ShaderHandle get_handle() { return handle; }
        ConstantHandle get_param_handle() { return param_handle; }
        ShaderLayout &get_layout() { return layout; }
        ~Shader() {
            RHI::dealloc(handle);
            if (param_handle != NULL_HANDLE) RHI::dealloc(param_handle);
        }
};
}  // namespace Seed

#endif