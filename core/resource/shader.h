#ifndef _SEED_SHADER_H_
#define _SEED_SHADER_H_

#include <vector>
#include "core/container/kstring.h"
#include "core/handle.h"
#include "core/io/path.h"
#include "core/rendering/rhi/render_resource.h"
#include "core/resource/resource.h"
#include "core/rendering/shader_layout.h"

namespace Seed {
struct ShaderDefine {
        KString name;
        KString value;
};

class Shader : public Resource {
    private:
        ShaderHandle handle;
        ConstantHandle param_handle = NULL_HANDLE;
        ShaderLayout layout;
        std::vector<ShaderDefine> defines;
        u32 version = 0;

    public:
        Shader(const Path &path, const KString &code);
        Shader(const Path &path, const KString &code,
               const std::vector<ShaderDefine> &defines);
        ShaderHandle get_handle() { return handle; }
        ConstantHandle get_param_handle() { return param_handle; }
        ShaderLayout &get_layout() { return layout; }
        bool reload_from_disk();
        Ref<Shader> create_variant(const std::vector<ShaderDefine> &defines);
        u32 get_version() { return version; }
        ~Shader();
};
}  // namespace Seed

#endif
