#include "shader_proxy.h"
#include "core/rendering/api/render_engine.h"
#include "core/rendering/backend/opengl_backend.h"
#include "core/rendering/backend/vulkan_backend.h"
#include "core/io/file.h"
#include <filesystem>
#include "core/rendering/shader_layout.h"

namespace Seed {

// SlangResult ShaderProxy::SeedFileSystem::loadFile(char const *path,
//                                                   ISlangBlob **outBlob) {
//     Ref<File> file = File::open(path);
//     if (file.is_null()) {
//         return SLANG_E_NOT_FOUND;
//     }
//     // auto content = file->read_vector();
//     RawBlob blob;
//     // *outBlob = RawBlob
//     return SLANG_OK;
// }

static std::vector<slang::CompilerOptionEntry> spirv_compile_opt = {
    slang::CompilerOptionEntry{
        .name = slang::CompilerOptionName::EmitSpirvDirectly,
        .value =
            slang::CompilerOptionValue{
                .kind = slang::CompilerOptionValueKind::Int,
                .intValue0 = true}},
    slang::CompilerOptionEntry{
        .name = slang::CompilerOptionName::VulkanUseEntryPointName,
        .value = slang::CompilerOptionValue{
            .kind = slang::CompilerOptionValueKind::Int, .intValue0 = true}}};

ShaderProxy::ShaderProxy(const std::vector<std::string> &include_path) {
    for (auto &path : include_path) {
        char *_path = (char *)malloc(path.size() + 1);
        memcpy(_path, path.c_str(), path.size() + 1);
        this->include_path.push_back(_path);
    }

    slang::createGlobalSession(global_session.writeRef());

    glsl_target_desc.format = SLANG_GLSL;
    glsl_target_desc.profile = global_session->findProfile("glsl_430");
    glsl_session_desc.targets = &glsl_target_desc;
    glsl_session_desc.targetCount = 1;
    glsl_session_desc.searchPaths = this->include_path.data();
    glsl_session_desc.searchPathCount = this->include_path.size();
    glsl_session_desc.flags |= SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM;
    // glsl_session_desc.fileSystem = &this->file_system;

    spirv_target_desc.format = SLANG_SPIRV;
    spirv_target_desc.profile = global_session->findProfile("spirv_1_3");
    spirv_session_desc.targets = &spirv_target_desc;
    spirv_session_desc.targetCount = 1;
    spirv_session_desc.searchPaths = this->include_path.data();
    spirv_session_desc.searchPathCount = this->include_path.size();
    spirv_session_desc.compilerOptionEntries = spirv_compile_opt.data();
    spirv_session_desc.compilerOptionEntryCount = spirv_compile_opt.size();
    // spirv_session_desc.fileSystem = &this->file_system;
}

void ShaderProxy::append_binding_set(slang::TypeLayoutReflection *layout,
                                     ShaderLayout &shader_layout) {
    u32 binding_cnt = layout->getBindingRangeCount();
    u32 push_constant_offset = 0;
    ShaderBindingSet binding_set;
    for (uint32_t i = 0; i < binding_cnt; i++) {
        slang::BindingType _type = layout->getBindingRangeType(i);
        std::string name = layout->getBindingRangeLeafVariable(i)->getName();
        i64 count = layout->getBindingRangeBindingCount(i);

        i64 set = layout->getBindingRangeDescriptorSetIndex(i);
        i64 descriptorRangeIndex =
            layout->getBindingRangeFirstDescriptorRangeIndex(i);
        i64 binding = layout->getDescriptorSetDescriptorRangeIndexOffset(
            set, descriptorRangeIndex);
        shader_layout.texture_unit.emplace(name, binding);
        switch (_type) {
            case slang::BindingType::CombinedTextureSampler:
            case slang::BindingType::Sampler:
                binding_set.bindings.push_back(
                    ShaderBinding{.binding_point = binding,
                                  .type = ShaderResourceType::SAMPLER,
                                  .count = count,
                                  .name = name});
                break;
            case slang::BindingType::ConstantBuffer:
                binding_set.bindings.push_back(
                    ShaderBinding{.binding_point = binding,
                                  .type = ShaderResourceType::UBO,
                                  .count = count,
                                  .name = name});
                break;
            case slang::BindingType::RawBuffer:
            case slang::BindingType::MutableRawBuffer:
                binding_set.bindings.push_back(
                    ShaderBinding{.binding_point = binding,
                                  .type = ShaderResourceType::SSBO,
                                  .count = count,
                                  .name = name});
                break;
            case slang::BindingType::ParameterBlock:
                append_binding_set(layout->getBindingRangeLeafTypeLayout(i)
                                       ->getElementTypeLayout(),
                                   shader_layout);
                break;
            case slang::BindingType::PushConstant: {
                u32 cnt = layout->getBindingRangeLeafTypeLayout(i)
                              ->getElementTypeLayout()
                              ->getFieldCount();
                u32 size = layout->getBindingRangeLeafTypeLayout(i)
                               ->getElementTypeLayout()
                               ->getSize();
                if (size > 256) {
                    spdlog::warn(
                        "Shader '{}.slang' push contant size {} exceeds 256",
                        name, size);
                }
                size &= 0xff;
                // u64 offset =
                // globlalLayout->getBindingRangeLeafTypeLayout(i)->get();
                shader_layout.push_constants.push_back(PushConstantRange{
                    .offset = (u8)(push_constant_offset & 0xff),
                    .size = (u8)(size)});
                push_constant_offset += size;
                break;
            }
            default:
                break;
        }
    }

    shader_layout.sets.push_back(binding_set);
}

void ShaderProxy::compile_shader(RenderResource *rc, const std::string &path,
                                 const std::string &shader,
                                 ShaderLayout *layout) {
    RenderBackend *backend = RenderEngine::get_instance()->get_device();
    auto get_module_name = [](const std::string &path) -> std::string {
        std::filesystem::path p(path);
        return p.stem().string();
    };
    Slang::ComPtr<slang::ISession> session;
    std::string module_name = get_module_name(path);
    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IModule> module;
    switch (backend->get_type()) {
        case RenderBackendType::OPENGL:
            global_session->createSession(glsl_session_desc,
                                          session.writeRef());

            break;
        case RenderBackendType::VULKAN:
            global_session->createSession(spirv_session_desc,
                                          session.writeRef());
            break;
        default:
            break;
    }

    module = session->loadModuleFromSourceString(
        module_name.data(), path.data(), shader.data(), diagnostics.writeRef());
    if (diagnostics) {
        SPDLOG_ERROR("Slang shader diagnostic: {}",
                     (const char *)diagnostics->getBufferPointer());
    }
    std::vector<slang::IComponentType *> com_types;
    com_types.push_back(module);

    std::vector<EntryPointInfo> entry_points;
    auto add_entry = [&](const char *name, ShaderStage stage) {
        Slang::ComPtr<slang::IEntryPoint> ep;
        if (module->findEntryPointByName(name, ep.writeRef()) == SLANG_OK) {
            entry_points.push_back(
                {stage, static_cast<u32>(entry_points.size())});
            com_types.push_back(ep);
        }
    };

    add_entry("vert", ShaderStage::VERTEX);
    add_entry("tesc", ShaderStage::TESS_CTRL);
    add_entry("tese", ShaderStage::TESS_EVAL);
    add_entry("geom", ShaderStage::GEOMETRY);
    add_entry("frag", ShaderStage::FRAGMENT);

    Slang::ComPtr<slang::IComponentType> program;
    session->createCompositeComponentType(com_types.data(), com_types.size(),
                                          program.writeRef());
    diagnostics = nullptr;
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    program->link(linkedProgram.writeRef(), diagnostics.writeRef());
    if (diagnostics) {
        SPDLOG_ERROR("Slang shader diagnostic: {}",
                     (const char *)diagnostics->getBufferPointer());
    }
    slang::ProgramLayout *programLayout = linkedProgram->getLayout(0);
    slang::TypeLayoutReflection *globlalLayout =
        programLayout->getGlobalParamsTypeLayout();
    ShaderLayout _layout;
    append_binding_set(globlalLayout, _layout);
    std::reverse(_layout.sets.begin(), _layout.sets.end());

    std::string vert;
    std::string tesc;
    std::string tese;
    std::string geom;
    std::string frag;
    for (auto &ep : entry_points) {
        Slang::ComPtr<slang::IBlob> code;
        Slang::ComPtr<slang::IBlob> diagnostics;
        linkedProgram->getEntryPointCode(ep.index, 0, code.writeRef(),
                                         diagnostics.writeRef());
        if (diagnostics) {
            SPDLOG_ERROR("Slang shader diagnostic: {}",
                         (const char *)diagnostics->getBufferPointer());
        }
        switch (ep.stage) {
            case ShaderStage::VERTEX:
                vert.assign((char *)code->getBufferPointer(),
                            code->getBufferSize());
                break;
            case ShaderStage::TESS_CTRL:
                tesc.assign((char *)code->getBufferPointer(),
                            code->getBufferSize());

                break;
            case ShaderStage::TESS_EVAL:
                tese.assign((char *)code->getBufferPointer(),
                            code->getBufferSize());

                break;
            case ShaderStage::GEOMETRY:
                geom.assign((char *)code->getBufferPointer(),
                            code->getBufferSize());

                break;
            case ShaderStage::FRAGMENT:
                frag.assign((char *)code->getBufferPointer(),
                            code->getBufferSize());

                break;
        }
    }

    RenderEngine::get_instance()->get_device()->alloc_shader(rc, vert, frag,
                                                             geom, tesc, tese);
    RenderEngine::get_instance()->get_device()->setup_shader_layout(rc,
                                                                    _layout);
    if (layout) {
        *layout = _layout;
    }
}

ShaderProxy::~ShaderProxy() {
    for (auto path : include_path) {
        free(path);
    }
}

}  // namespace Seed