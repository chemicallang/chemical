// Copyright (c) Chemical Language Foundation 2026.

#include <iostream>

#include "ChildProcessBuild.h"
#include "ContextSerialization.h"
#include "server/WorkspaceManager.h"
#include "utils/PathUtils.h"
#include "utils/FileUtils.h"
#include "compiler/lab/LabBuildCompiler.h"
#include "integration/libtcc/LibTccInteg.h"

int compile_lab(
    const std::string& exe_path,
    const std::string& lab_path,
    std::string& outPayload,
    bool format
) {

    // using is64Bit as true
    auto is64Bit = true;
    auto compiler_exe_path = exe_path;
    auto is_mod_source = lab_path.ends_with(".mod");
    auto compiler_build_dir = resolve_sibling(lab_path, "build");
    auto build_dir = resolve_rel_child_path_str(compiler_build_dir, "ide");
    // create build directory before proceeding (if it doesn't exist)
    create_dir(compiler_build_dir);
    create_dir(build_dir);

    CompilerBinder binder(compiler_exe_path);
    LocationManager loc_man;

    // creating the compiler (static function, cannot reuse global contaienr)
    LabBuildCompilerOptions options(compiler_exe_path, "", build_dir, is64Bit);
    LabBuildCompiler compiler(loc_man, binder, &options);

    // creating context, this allows separation, we don't want to reuse
    // we do not want to share data between labs because lab files are very flexible
    LabBuildContext context(compiler, compiler.path_handler, compiler.mod_storage, binder);

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // the allocators that will be used for all jobs
    compiler.set_allocators(&_job_allocator, &_mod_allocator, &_file_allocator);

    // build the lab file to a tcc state
    const auto state = compiler.built_lab_file(context, lab_path, is_mod_source);

    // auto delte the tcc state
    TCCDeletor auto_delete(state);

    // get the build method
    auto build = (LabModule*(*)(LabBuildContext*, LabJob*)) tcc_get_symbol(state, "chemical_lab_build");
    if(!build) {
        std::cerr << "[lsp] there's no build function in the file" << std::endl;
        return 1;
    }

    // clear the module storage
    // these modules were created to facilitate the build.lab generation
    // if not cleared, these modules will interfere with modules created for executable
    compiler.mod_storage.clear();
    compiler.controller.clear();
    context.storage.clear();

    LabJob final_job(LabJobType::Executable, chem::string("lsp_build_job"), OutputMode::Debug);
    LabBuildContext::initialize_job(&final_job, &options);

    // call the root build.lab build's function
    const auto rootMod = build(&context, &final_job);

    // reporting build context info to parent
    auto info = BuildContextInformation {
        .root_module = rootMod,
        .modStorage = compiler.mod_storage,
        .jobs = std::move(compiler.executables),
        .binder = compiler.binder,
        .allocator = ASTAllocator(10000)
    };

    outPayload = labBuildContext_toJsonStr(info, format);

    return 0;

}
