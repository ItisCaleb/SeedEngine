#include "shader_proxy.h"
#include "core/rendering/api/render_engine.h"
#include "core/io/file.h"
#include <filesystem>

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
    spirv_target_desc.profile = global_session->findProfile("glsl_450");
    spirv_session_desc.targets = &spirv_target_desc;
    spirv_session_desc.targetCount = 1;
    spirv_session_desc.searchPaths = this->include_path.data();
    spirv_session_desc.searchPathCount = this->include_path.size();
    // spirv_session_desc.fileSystem = &this->file_system;
}

void ShaderProxy::compile_shader(RenderResource *rc, const std::string &path,
                                 const std::string &shader) {
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
        spdlog::error("Slang shader diagnostic: {}",
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
        spdlog::error("Slang shader diagnostic: {}",
                      (const char *)diagnostics->getBufferPointer());
    }
    switch (backend->get_type()) {
        case RenderBackendType::OPENGL:
            compile_glsl(rc, linkedProgram, entry_points);
            break;
        case RenderBackendType::VULKAN:
            compile_glsl(rc, linkedProgram, entry_points);
            break;
        default:
            break;
    }
}

void ShaderProxy::compile_glsl(RenderResource *rc,
                               Slang::ComPtr<slang::IComponentType> program,
                               const std::vector<EntryPointInfo> &entryPoints) {
    std::string vert;
    std::string tesc;
    std::string tese;
    std::string geom;
    std::string frag;
    for (auto &ep : entryPoints) {
        Slang::ComPtr<slang::IBlob> code;
        Slang::ComPtr<slang::IBlob> diagnostics;
        try{
            program->getEntryPointCode(ep.index, 0, code.writeRef(), diagnostics.writeRef());

        }catch(std::exception &e){
            spdlog::error(e.what());
        }
        if (diagnostics) {
            spdlog::error("Slang shader diagnostic: {}",
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
}
void ShaderProxy::compile_spirv(
    RenderResource *rc, Slang::ComPtr<slang::IComponentType> program,
    const std::vector<EntryPointInfo> &entryPoints) {}

ShaderProxy::~ShaderProxy() {
    for (auto path : include_path) {
        free(path);
    }
}

}  // namespace Seed