#ifndef _SEED_SHADER_PROXY_H_
#define _SEED_SHADER_PROXY_H_
#include <vector>
#include <string>
#include <slang.h>
#include <slang-com-ptr.h>
#include <slang-com-helper.h>
#include <unordered_map>
#include "core/rendering/api/render_resource.h"

namespace Seed {

struct ShaderBinding;
struct ShaderBindingSet;
struct PushConstantRange;

class ShaderProxy {
    private:
        std::vector<char *> include_path;
        Slang::ComPtr<slang::IGlobalSession> global_session;
        slang::TargetDesc glsl_target_desc;
        slang::TargetDesc spirv_target_desc;
        slang::SessionDesc glsl_session_desc;
        slang::SessionDesc spirv_session_desc;
        // struct RawBlob : public ISlangBlob {
        //         SLANG_IUNKNOWN_QUERY_INTERFACE
        //         SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE {
        //             return 1;
        //         }
        //         SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE
        //         {
        //             return 1;
        //         }
        //         virtual SLANG_NO_THROW void const *SLANG_MCALL
        //         getBufferPointer() override;
        //         virtual SLANG_NO_THROW size_t SLANG_MCALL
        //         getBufferSize() override;
        //         ISlangUnknown *getInterface(const Slang::Guid &guid) {
        //             return static_cast<ISlangBlob *>(this);
        //         }
        // };

        // struct SeedFileSystem : public ISlangFileSystem {
        //         SLANG_IUNKNOWN_QUERY_INTERFACE
        //         SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE {
        //             return 1;
        //         }
        //         SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE
        //         {
        //             return 1;
        //         }
        //         virtual SLANG_NO_THROW void *SLANG_MCALL
        //         castAs(const Slang::Guid &guid) SLANG_OVERRIDE;
        //         virtual SLANG_NO_THROW SlangResult SLANG_MCALL
        //         loadFile(char const *path, ISlangBlob **outBlob) override;
        //         ISlangUnknown *getInterface(const Slang::Guid &guid) {
        //             return static_cast<ISlangFileSystem *>(this);
        //         }
        // } file_system;

        enum class ShaderStage {
            VERTEX,
            TESS_CTRL,
            TESS_EVAL,
            GEOMETRY,
            FRAGMENT
        };

        struct EntryPointInfo {
                ShaderStage stage;
                u32 index;
        };

        void append_binding_set(slang::TypeLayoutReflection *reflection,
                                ShaderLayout &shader_layout);

    public:
        ShaderProxy(const std::vector<std::string> &include_path);
        ShaderHandle compile_shader(const std::string &path,
                                    const std::string &shader,
                                    ShaderLayout *layout);
        ~ShaderProxy();
};
}  // namespace Seed

#endif