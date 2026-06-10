#include "shader_proxy.h"
#include "core/container/kstring.h"
#include "core/rendering/rhi/render_engine.h"
#include <filesystem>
#include "core/rendering/rhi/shader_proxy.h"
#include "core/rendering/shader_layout.h"
#include "core/types.h"

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
    /* the default is*/
    spirv_session_desc.defaultMatrixLayoutMode =
        SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
    // spirv_session_desc.fileSystem = &this->file_system;
}

void ShaderProxy::append_binding_set(slang::TypeLayoutReflection *layout,
                                     ShaderLayout &shader_layout) {
    u32 binding_cnt = layout->getBindingRangeCount();
    u32 push_constant_offset = 0;
    ShaderBindingSet binding_set;
    for (u32 i = 0; i < binding_cnt; i++) {
        slang::BindingType _type = layout->getBindingRangeType(i);
        std::string name = layout->getBindingRangeLeafVariable(i)->getName();
        i64 count = layout->getBindingRangeBindingCount(i);

        i64 set = layout->getBindingRangeDescriptorSetIndex(i);
        i64 descriptorRangeIndex =
            layout->getBindingRangeFirstDescriptorRangeIndex(i);
        i64 binding = layout->getDescriptorSetDescriptorRangeIndexOffset(
            set, descriptorRangeIndex);
        u32 size = layout->getBindingRangeLeafTypeLayout(i)
                       ->getElementTypeLayout()
                       ->getSize();
        switch (_type) {
            case slang::BindingType::CombinedTextureSampler:
            case slang::BindingType::Sampler:
                binding_set.bindings.push_back(
                    ShaderBinding{.binding_point = binding,
                                  .type = ShaderResourceType::SAMPLER,
                                  .count = count,
                                  .name = name});
                shader_layout.texture_unit.emplace(name, binding);
                break;
            case slang::BindingType::ConstantBuffer:
                binding_set.bindings.push_back(
                    ShaderBinding{.binding_point = binding,
                                  .type = ShaderResourceType::UBO,
                                  .count = count,
                                  .size = size,
                                  .name = name});
                break;
            case slang::BindingType::RawBuffer:
            case slang::BindingType::MutableRawBuffer:
                binding_set.bindings.push_back(
                    ShaderBinding{.binding_point = binding,
                                  .type = ShaderResourceType::SSBO,
                                  .count = count,
                                  .size = size,
                                  .name = name});
                break;
            case slang::BindingType::ParameterBlock: {
                /* we assume there is only one parameter block */
                slang::TypeLayoutReflection *element_layout =
                    layout->getBindingRangeLeafTypeLayout(i)
                        ->getElementTypeLayout();

                size_t total_size = size;

                append_binding_set(element_layout, shader_layout);
                if (total_size > 0) {
                    shader_layout.ubo_binding.binding = binding;
                    shader_layout.ubo_binding.total_size = total_size;
                    shader_layout.sets.back().bindings.push_back(ShaderBinding{
                        .binding_point = binding,
                        .type = ShaderResourceType::UBO,
                        .count = count,
                        .size = total_size,
                        .name = name,
                    });

                    for (u32 f = 0; f < element_layout->getFieldCount(); f++) {
                        slang::VariableLayoutReflection *field =
                            element_layout->getFieldByIndex(f);
                        u32 offset =
                            field->getOffset(SLANG_PARAMETER_CATEGORY_UNIFORM);
                        u32 size = field->getTypeLayout()->getSize(
                            SLANG_PARAMETER_CATEGORY_UNIFORM);
                        if (size > 0) {
                            shader_layout.ubo_binding
                                .members[field->getName()] =
                                BufferMember{.offset = offset, .size = size};
                        }
                    }
                }
                break;
            }
            case slang::BindingType::PushConstant: {
                u32 cnt = layout->getBindingRangeLeafTypeLayout(i)
                              ->getElementTypeLayout()
                              ->getFieldCount();
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

ShaderHandle ShaderProxy::compile_shader(const Path &path,
                                         const KString &shader,
                                         ShaderLayout *layout) {
    RenderBackend *backend = RenderEngine::get_instance()->get_device();
    Slang::ComPtr<slang::ISession> session;
    KStr module_name = path.filename_without_ext();
    Slang::ComPtr<slang::IBlob> diagnostics;
    Slang::ComPtr<slang::IModule> module;
    switch (backend->get_type()) {
        case RenderBackendType::VULKAN:
        case RenderBackendType::XR_VULKAN:
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
        return NULL_HANDLE;
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
        return NULL_HANDLE;
    }
    slang::ProgramLayout *programLayout = linkedProgram->getLayout(0);
    slang::TypeLayoutReflection *globlalLayout =
        programLayout->getGlobalParamsTypeLayout();
    ShaderLayout _layout;
    append_binding_set(globlalLayout, _layout);
    std::reverse(_layout.sets.begin(), _layout.sets.end());

    KString vert;
    KString tesc;
    KString tese;
    KString geom;
    KString frag;
    for (auto &ep : entry_points) {
        Slang::ComPtr<slang::IBlob> code;
        Slang::ComPtr<slang::IBlob> diagnostics;
        linkedProgram->getEntryPointCode(ep.index, 0, code.writeRef(),
                                         diagnostics.writeRef());
        if (diagnostics) {
            SPDLOG_ERROR("Slang shader diagnostic: {}",
                         (const char *)diagnostics->getBufferPointer());
            return NULL_HANDLE;
        }
        switch (ep.stage) {
            case ShaderStage::VERTEX:
                vert.append(KStr((char *)code->getBufferPointer(),
                                 code->getBufferSize()));
                break;
            case ShaderStage::TESS_CTRL:
                tesc.append(KStr((char *)code->getBufferPointer(),
                                 code->getBufferSize()));
                break;
            case ShaderStage::TESS_EVAL:
                tese.append(KStr((char *)code->getBufferPointer(),
                                 code->getBufferSize()));

                break;
            case ShaderStage::GEOMETRY:
                geom.append(KStr((char *)code->getBufferPointer(),
                                 code->getBufferSize()));

                break;
            case ShaderStage::FRAGMENT:
                frag.append(KStr((char *)code->getBufferPointer(),
                                 code->getBufferSize()));

                break;
        }
    }

    ShaderHandle handle =
        RenderEngine::get_instance()->get_device()->alloc_shader(
            vert, frag, geom, tesc, tese);
    RenderEngine::get_instance()->get_device()->setup_shader_layout(handle,
                                                                    _layout);
    if (layout) {
        *layout = _layout;
    }
    return handle;
}

ShaderProxy::~ShaderProxy() {
    for (auto path : include_path) {
        free(path);
    }
}

}  // namespace Seed