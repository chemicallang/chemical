// Copyright (c) Chemical Language Foundation 2025.

#include "rang.hpp"
#include "LabBuildCompiler.h"
#include "ast/types/LinkedType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/Import.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/If.h"
#include "utils/Benchmark.h"
#include "ast/structures/ModuleScope.h"
#include "Utils.h"
#include "utils/ProcessUtils.h"
#include "core/source/LocationManager.h"
#include <fstream>
#include <span>
#include "compiler/lab/mod_conv/ModToLabConverter.h"
#include "parser/utils/ParseModDecl.h"
#include "compiler/lab/timestamp/Timestamp.h"
#include "std/chem_string_view_fast.h"
#include "TargetConditionAPI.h"
#ifdef COMPILER_BUILD
#include "compiler/Codegen.h"
#endif
#include "parser/Parser.h"
#include "compiler/SymbolResolver.h"
#include "compiler/ASTProcessor.h"
#include "ast/structures/Namespace.h"
#include "preprocess/2c/2cASTVisitor.h"
#include "compiler/lab/LabBuildContext.h"
#include "integration/libtcc/LibTccInteg.h"
#include "ctpl.h"
#include "compiler/InvokeUtils.h"
#include "utils/PathUtils.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>
#include <filesystem>
#include <iostream>
#include <utility>
#include <functional>
#include "utils/FileUtils.h"
#include "compiler/backend/LLVMBackendContext.h"
#include "preprocess/2c/2cBackendContext.h"
#include "compiler/cbi/model/CompilerBinder.h"
#include "compiler/SelfInvocation.h"
#include "utils/CmdUtils.h"
#include "utils/CmdUtils.h"
#include "ast/base/TypeBuilder.h"
#include <mutex>
#include <future>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <thread>
#include <chrono>
#include "compiler/lab/transformer/TransformerContext.h"



#ifdef COMPILER_BUILD
#include "compiler/ctranslator/CTranslator.h"
#include "core/source/LocationManager.h"

#endif

std::ostream& operator<<(std::ostream& os, const LabModule& mod) {
    if(mod.scope_name.empty()) {
        os << mod.name;
    } else {
        os << mod.scope_name << ':' << mod.name;
    }
    return os;
}

TCCMode to_tcc_mode(OutputMode mode, bool debug_info) {
    if(debug_info) {
        switch(mode) {
            case OutputMode::DebugComplete:
                return TCCMode::DebugComplete;
            default:
                return TCCMode::Debug;
        }
    }
    switch(mode) {
        case OutputMode::Debug:
        case OutputMode::DebugQuick:
            return TCCMode::Debug;
        case OutputMode::DebugComplete:
            return TCCMode::DebugComplete;
        case OutputMode::ReleaseFast:
        case OutputMode::ReleaseSmall:
        case OutputMode::ReleaseSafe:
            return TCCMode::None;
        default:
            return TCCMode::None;
    }
}

inline TCCMode to_tcc_mode(LabBuildCompilerOptions* options) {
    return to_tcc_mode(options->out_mode, options->debug_info);
}

static bool verify_lib_build_func_type(FunctionDeclaration* found, const std::string& abs_path) {
    if(found->returnType->kind() == BaseTypeKind::Pointer) {
        auto child_type = found->returnType->known_child_type();
        if(child_type->linked_name() == "Module") {
            return true;
        }
    }
    std::cerr << "[lab] lab file at " << abs_path << " is a library, so it's build method's signature should return a Module*" << std::endl;
    return false;
}

static bool verify_app_build_func_type(FunctionDeclaration* found, const std::string& abs_path) {
    if(found->returnType->kind() != BaseTypeKind::Void) {
        std::cerr << "[lab] the root .lab file at " << abs_path << " provided to compiler should use a void return type in it's build method" << std::endl;
        return false;
    }
    return true;
}

void recursive_dedupe(LabModule* file, std::unordered_map<LabModule*, bool>& imported, std::vector<LabModule*>& flat_map) {
    for(auto& nested : file->get_dependencies()) {
        recursive_dedupe(nested.module, imported, flat_map);
    }
    auto found = imported.find(file);
    if(found == imported.end()) {
        imported[file] = true;
        flat_map.emplace_back(file);
    }
}

/**
 * same as above, only it operates on multiple modules, it de-dupes the dependencies modules
 * of the given list of modules and also sorts them
 */
std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<ModuleDependency>& dependencies) {
    std::vector<LabModule*> new_modules;
    std::unordered_map<LabModule*, bool> imported;
    for(auto& dep : dependencies) {
        recursive_dedupe(dep.module, imported, new_modules);
    }
    return new_modules;
}

/**
 * this should be used in each job, so children of each module are calculated again
 */
void zero_out_cached_children(std::vector<LabModule*>& mods) {
    for(const auto mod : mods) {
        mod->children = nullptr;
    }
}

/**
 * constructor
 */
LabBuildCompiler::LabBuildCompiler(
    LocationManager& loc_man,
    CompilerBinder& binder,
    LabBuildCompilerOptions* options,
    int nThreads
) : path_handler(options->exe_path), loc_man(loc_man), binder(binder), options(options), pool(nThreads),
global_allocator(100000 /** 100 kb**/), type_builder(global_allocator) {

}

LabBuildCompiler::LabBuildCompiler(
        LocationManager& loc_man,
        CompilerBinder& binder,
        LabBuildCompilerOptions *options
) : LabBuildCompiler(loc_man, binder, options, (int) std::thread::hardware_concurrency())
{

}

int LabBuildCompiler::do_job(LabJob* job) {

    // switch the mode to current job's mode
    auto previous_mode = options->out_mode;
    options->out_mode = job->mode;

    const auto bm = options->benchmark;
    current_job = job;

    // benchmark
    BenchmarkResults bm_res;
    if(bm) {
        bm_res.benchmark_begin();
    }

    job->status = LabJobStatus::Launched;
    int return_int;
    switch(job->type) {
        case LabJobType::Executable:
            return_int = do_executable_job(job);
            break;
        case LabJobType::JITExecutable:
            return_int = process_modules(job);
            break;
        case LabJobType::Library:
            return_int = do_library_job(job);
            break;
        case LabJobType::ToCTranslation:
        case LabJobType::ProcessingOnly:
        case LabJobType::Intermediate:
        case LabJobType::CBI:
            return_int = process_modules(job);
            break;
        case LabJobType::ToChemicalTranslation:
            return_int = do_to_chemical_job(job);
            break;
    }
    if(job->status == LabJobStatus::Launched) {
        if(return_int == 0) {
            job->status = LabJobStatus::Success;
        } else {
            job->status = LabJobStatus::Failure;
        }
    }

    // end and print the benchmark for module
    if(bm) {
        bm_res.benchmark_end();
        ASTProcessor::print_benchmarks(std::cout, "bm:job", job->name.to_view(), &bm_res);
    }

    options->out_mode = previous_mode;

    return return_int;
}

typedef void(*ImportCycleHandler)(void* data_ptr, std::vector<unsigned int>& parents, ASTFileResult* imported_file, ASTFileResult* parent, bool direct);

void check_imports_for_cycles(void* data_ptr, ASTFileResult* parent_file, std::vector<unsigned int>& parents, ImportCycleHandler handler) {
    auto end_size = parents.size();
    for(auto& importedFile : parent_file->imports) {
        const auto imported = importedFile.result;
        // checking for direct cyclic dependency a imports a
        if(imported->file_id == parent_file->file_id) {
            // found direct cyclic dependency
            handler(data_ptr, parents, imported, parent_file, true);
        } else {
            // clear the parents after end_itr so sibling import trees don't conflict
            parents.erase(parents.begin() + end_size, parents.end());
            // check file against all parents to find indirect cycling dependencies
            bool has_indirect_dep = false;
            for(const auto id : parents) {
                if(imported->file_id == id) {
                    // found cycling dependency
                    handler(data_ptr, parents, imported, parent_file, false);
                    has_indirect_dep = true;
                }
            }
            // add parent file so it get's checked in nested tree
            parents.emplace_back(parent_file->file_id);
            // cycle may exist in imports of this file
            if(!has_indirect_dep) {
                // only check in this file's imports if there's no cycle otherwise stackoverflow in this function
                check_imports_for_cycles(data_ptr, imported, parents, handler);
            }
        }
    }
}

void check_imports_for_cycles(void* data_ptr, const std::span<ASTFileResult*>& files, ImportCycleHandler handler) {
    std::vector<unsigned int> parents;
    parents.reserve(16);
    for(const auto file : files) {
        parents.clear();
        check_imports_for_cycles(data_ptr, file, parents, handler);
    }
}

struct ImportCycleCheckResult {

    bool has_cyclic_dependencies;

    LocationManager& loc_man;

};

void check_imports_for_cycles(ImportCycleCheckResult& out, const std::span<ASTFileResult*>& module_files) {
    check_imports_for_cycles(&out, module_files, [](void* data_ptr, std::vector<unsigned int>& parents, ASTFileResult* imported_file, ASTFileResult* parent_file, bool direct){
        const auto holder = (ImportCycleCheckResult*) data_ptr;
        auto& locMan = holder->loc_man;
        holder->has_cyclic_dependencies = true;
        const auto file_path = parent_file->abs_path;
        std::cerr << rang::fg::red << "Cyclic dependency detected, file '" << file_path << "' imported by '" << imported_file->abs_path << "'" << rang::fg::reset << std::endl;
        if(!direct) {
            int i = parents.size() - 1;
            while(i >= 0) {
                const auto next = locMan.getPathForFileId(parents[i]);
                std::cerr << rang::fg::red << "which is imported by '" << next << "'" << rang::fg::reset << std::endl;
                i--;
            }
        }
    });
}

void flatten(std::vector<ASTFileResult*>& flat_out, std::unordered_map<chem::string_view, bool, FastHashInternedView, FastEqInternedView>& done_files, ASTFileResult* single_file) {
    for(auto& file : single_file->imports) {
        flatten(flat_out, done_files, file.result);
    }
    auto view = chem::string_view(single_file->abs_path);
    auto found = done_files.find(view);
    if(found == done_files.end()) {
        done_files[view] = true;
        flat_out.emplace_back(single_file);
    }
}

std::vector<ASTFileResult*> flatten(const std::span<ASTFileResult*>& files) {
    std::vector<ASTFileResult*> flat_out;
    std::unordered_map<chem::string_view, bool, FastHashInternedView, FastEqInternedView> done_files;
    for(auto file : files) {
        flatten(flat_out, done_files, file);
    }
    return flat_out;
}

namespace fs = std::filesystem;

bool determine_change_in_files(LabBuildCompiler* compiler, LabModule* mod, const std::string& mod_timestamp_file) {

    auto& direct_files = mod->direct_files;
    const auto verbose = compiler->options->verbose;
    const auto caching = compiler->options->is_caching_enabled;

    if(verbose) {
        std::cout << "[lab] " << "checking if module " << mod->scope_name << ':' << mod->name << " has changed" << std::endl;
    }

    if(mod->type == LabModuleType::CFile || mod->type == LabModuleType::CPPFile) {

        std::vector<std::string_view> paths;
        for(auto& path : mod->paths) {
            paths.emplace_back(path.to_view());
        }

        // let's check if module timestamp file exists and is valid (files haven't changed)
        if (compare_mod_timestamp(paths, mod_timestamp_file, compiler->options->out_mode)) {

            if (verbose) {

                std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "'" << std::endl;

            }

            return false;

        } else if (verbose) {

            std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;

        }

    } else {

        // let's check if module timestamp file exists and is valid (files haven't changed)
        if (compare_mod_timestamp(direct_files, mod_timestamp_file, compiler->options->out_mode)) {

            if (verbose) {

                std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "'" << std::endl;

            }

            return false;

        } else if (verbose) {

            std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;

        }

    }

    return true;

}

std::string get_mod_dir(LabJob* job, LabModule* mod) {
    auto dir = std::string("modules/");
    LabModule::format(dir, mod->scope_name.to_chem_view(), mod->name.to_chem_view(), '.');
    return resolve_rel_child_path_str(job->build_dir.to_view(), dir);
}

std::string get_mod_timestamp_path(const std::string_view& build_dir, LabModule* mod, bool use_tcc) {
    auto f = mod->format('.');
    f.append(use_tcc ? "/timestamp_tcc.dat" : "/timestamp.dat");
    return resolve_rel_child_path_str(build_dir, f);
}

std::string get_partial_c_path(const std::string_view& build_dir, LabModule* mod) {
    auto f = mod->format('.');
    f.append("/partial.2c.c");
    return resolve_rel_child_path_str(build_dir, f);
}

bool has_module_changed_recursive(LabBuildCompiler* compiler, LabModule* module, const std::string& build_dir, bool use_tcc) {
    const auto verbose = compiler->options->verbose;
    if(use_tcc && module->type == LabModuleType::CPPFile) {
        // since cpp file modules are skipped in tiny cc build
        // so we will ignore and assume it hasn't changed
        return false;
    }
    if(use_tcc && module->type != LabModuleType::CFile) {
        // asked to check the partial c file
        auto partial_c = get_partial_c_path(build_dir, module);
        if(!fs::exists(partial_c)) {
            if (verbose) {
                std::cout << "[lab] " << "couldn't find partial c file at '" << partial_c << "' for module '" << *module << std::endl;
            }
            return true;
        } else {
            if(verbose) {
                std::cout << "[lab] " << "found cached partial c at '" << partial_c << "'" << std::endl;
            }
        }
    } else {
        // asked to check the object file
        if (!fs::exists(module->object_path.to_view())) {
            if (verbose) {
                std::cout << "[lab] " << "couldn't find cached object file at '" << module->object_path << "' for module '" << *module << std::endl;
            }
            return true;
        } else {
            if (verbose) {
                std::cout << "[lab] " << "found cached object file at '" << module->object_path << "'" << std::endl;
            }
        }
    }
    bool has_deps_changed = false;
    for(auto& dep : module->dependencies) {
        if(has_module_changed_recursive(compiler, dep.module, build_dir, use_tcc)) {
            has_deps_changed = true;
        }
    }
    if(has_deps_changed) {
        module->has_changed = true;
        return true;
    }
    if(module->has_changed.has_value()) {
        return module->has_changed.value();
    }
    auto mod_timestamp_file = get_mod_timestamp_path(build_dir, module, use_tcc);
    const auto has_changed = determine_change_in_files(compiler, module, mod_timestamp_file);
    module->has_changed = has_changed;
    return has_changed;
}

bool has_module_changed(LabBuildCompiler* compiler, LabModule* module, const std::string& build_dir, bool use_tcc) {
    const auto caching = compiler->options->is_caching_enabled;
    const auto verbose = compiler->options->verbose;
    if(!caching) {
        if(verbose) {
            std::cout << "[lab] " << "skipping cache use, caching disabled" << std::endl;
        }
        return true;
    }
    return has_module_changed_recursive(compiler, module, build_dir, use_tcc);
}

bool determine_if_files_have_changed(LabBuildCompiler* compiler, const std::vector<ASTFileResult*>& files, const std::string_view& object_path, const std::string& mod_timestamp_file) {

    const auto verbose = compiler->options->verbose;

    if(fs::exists(object_path)) {

        if (verbose) {
            std::cout << "[lab] " << "found cached object file '" << object_path << "'" << std::endl;
        }

        // let's check if module timestamp file exists and is valid (files haven't changed)
        if (compare_mod_timestamp(files, mod_timestamp_file, compiler->options->out_mode)) {

            if (verbose) {

                std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "'" << std::endl;

            }

            return false;

        } else if (verbose) {

            std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;

        }

    } else if(verbose) {

        std::cout << "[lab] " << "couldn't find cached object file at '" << object_path << std::endl;

    }

    return true;

}

void set_defined_declarations(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::NamespaceDecl:{
            const auto ns = node->as_namespace_unsafe();
            for(const auto child : ns->nodes) {
                set_defined_declarations(child);
            }
            break;
        }
        case ASTNodeKind::IfStmt: {
            const auto stmt = node->as_if_stmt_unsafe();
            if(stmt->computed_scope.has_value()) {
                const auto scope = stmt->computed_scope.value();
                if(scope) {
                    for(const auto child : scope->nodes) {
                        set_defined_declarations(child);
                    }
                }
            }
            break;
        }
        case ASTNodeKind::StructDecl:{
            const auto decl = node->as_struct_def_unsafe();
            decl->has_declared = true;
            break;
        }
        case ASTNodeKind::UnionDecl:{
            const auto decl = node->as_union_def_unsafe();
            decl->has_declared = true;
            break;
        }
        case ASTNodeKind::VariantDecl:{
            const auto decl = node->as_variant_def_unsafe();
            decl->has_declared = true;
            break;
        }
        case ASTNodeKind::TypealiasStmt: {
            const auto decl = node->as_typealias_unsafe();
            decl->has_declared = true;
            break;
        }
        default:
            break;
    }
}


void set_generated_instantiations(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::GenericFuncDecl: {
            const auto func = node->as_gen_func_decl_unsafe();
            func->total_bodied_instantiations = func->instantiations.size();
            break;
        }
        case ASTNodeKind::NamespaceDecl:{
            const auto ns = node->as_namespace_unsafe();
            for(const auto child : ns->nodes) {
                set_generated_instantiations(child);
            }
            break;
        }
        case ASTNodeKind::IfStmt: {
            const auto stmt = node->as_if_stmt_unsafe();
            if(stmt->computed_scope.has_value()) {
                const auto scope = stmt->computed_scope.value();
                if(scope) {
                    for(const auto child : scope->nodes) {
                        set_generated_instantiations(child);
                    }
                }
            }
            break;
        }
        case ASTNodeKind::InterfaceDecl: {
            // interfaces generate vtables for structs
            const auto interface = node->as_interface_def_unsafe();
#ifdef COMPILER_BUILD
            // indicate that functions have been generated
            for(const auto func : interface->instantiated_functions()) {
                for(auto& use : interface->users) {
                    auto& user = interface->users[use.first];
                    user[func] = { nullptr, false };
                }
            }
            // indicate vtables have been generated
            for(auto& user : interface->users) {
                interface->vtable_pointers[user.first] = nullptr;
            }
#else
            // indicate that functions have been generated
            for(auto user : interface->users) {
                interface->users[user.first] = true;
            }
#endif
            break;
        }
        case ASTNodeKind::GenericStructDecl:{
            const auto decl = node->as_gen_struct_def_unsafe();
            const auto size = decl->instantiations.size();
            decl->total_declared_instantiations = size;
            decl->total_bodied_instantiations = size;
            break;
        }
        case ASTNodeKind::GenericUnionDecl:{
            const auto decl = node->as_gen_union_decl_unsafe();
            const auto size = decl->instantiations.size();
            decl->total_declared_instantiations = size;
            decl->total_bodied_instantiations = size;
            break;
        }
        case ASTNodeKind::GenericInterfaceDecl:{
            const auto decl = node->as_gen_interface_decl_unsafe();
            const auto size = decl->instantiations.size();
            for(const auto inst : decl->instantiations) {
                set_generated_instantiations(inst);
            }
            decl->total_declared_instantiations = size;
            decl->total_bodied_instantiations = size;
            break;
        }
        case ASTNodeKind::GenericVariantDecl:{
            const auto decl = node->as_gen_variant_decl_unsafe();
            const auto size = decl->instantiations.size();
            decl->total_declared_instantiations = size;
            decl->total_bodied_instantiations = size;
            break;
        }
        default:
            break;
    }
}

void process_cached_module(ASTProcessor& processor, std::vector<ASTFileMetaData>& files, bool is_tcc) {
    for(const auto& file : files) {
        auto& nodes = file.result->unit.scope.body.nodes;
        if(is_tcc) {
            for(const auto node : nodes) {
                set_generated_instantiations(node);
                set_defined_declarations(node);
            }
        } else {
            for(const auto node : nodes) {
                set_generated_instantiations(node);
            }
        }
    }
}

void remove_non_public_nodes(ASTProcessor& processor, std::vector<ASTFileMetaData>& module_files) {
    // going over each file in the module, to remove non-public nodes
    // so when we declare the nodes in other module, we don't consider non-public nodes
    // because non-public nodes are only present in the module allocator which will be cleared
    for(const auto& file : module_files) {
        file.result->unit.scope.body.make_exportable();
    }
}

int LabBuildCompiler::process_module_tcc_bm(
        LabModule* mod,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        const std::string_view& build_dir
) {

    const auto bm_mod = options->benchmark_modules;

    // benchmark for the module compilation
    BenchmarkResults bm;

    if(bm_mod) {
        bm.benchmark_begin();
    }

    // the actual translation happens here
    const auto result = process_module_tcc(mod, processor, c_visitor, build_dir);
    if(result != 0) {
        return result;
    }

    // end and print the benchmark for module
    if(bm_mod) {
        bm.benchmark_end();
        // printing the benchmark
        ASTProcessor::print_benchmarks(std::cout, "bm:module", mod->format(), &bm);
    }

    return 0;

}

int LabBuildCompiler::process_module_tcc(
        LabModule* mod,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        const std::string_view& build_dir
) {

    // variables
    const auto caching = options->is_caching_enabled;
    const auto verbose = options->verbose;
    const bool is_use_obj_format = options->use_mod_obj_format;

    auto& resolver = *processor.resolver;

    // direct files are stored inside the module
    auto& direct_files = mod->direct_files;

    // we always build the module
    std::cout << rang::bg::gray << rang::fg::black << "[lab] " << "Building module ";
    std::cout << *mod << rang::bg::reset << rang::fg::reset << std::endl;

    if(verbose) {
        std::cout << "[lab] " << "parsing the module '" << *mod << '\'' << std::endl;
    }

    // this would import these direct files (lex and parse), into the module files
    // the module files will have imports, any file imported (from this module or external module will be included)
    const auto parse_success = processor.import_module_files_direct(pool, direct_files, mod);

    // return failure if parse failed
    if(!parse_success) {
        if(verbose) {
            std::cout << "[lab] " << "parsing failure in the module " << *mod << std::endl;
        }
        return 1;
    }

    if(verbose) {
        std::cout << "[lab] " << "resolving symbols in the module " << *mod << std::endl;
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = processor.sym_res_module(mod);
    if(sym_res_status != 0) {
        std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "failure during symbol resolution in the module " << *mod << std::endl;
        return sym_res_status;
    }

    // type verify the module
    if(!processor.type_verify_module_parallel(pool, mod)) {
        if(verbose) {
            std::cout << "[lab] " << "failure during type verification in the module " << *mod << std::endl;
        }
        return 1;
    }

    // check if module has not changed, and use cache appropriately
    // not changed means object file is also present (we make the check when setting the boolean)
    if(mod->has_changed.has_value() && !mod->has_changed.value()) {

        if(verbose) {
            std::cout << "[lab] " << "module " << mod->scope_name << ':' << mod->name << " hasn't changed, skipping compilation" << std::endl;
        }

        // append the partial c output to buffered writer
        auto partial_c_out = get_partial_c_path(build_dir, mod);
        if(fs::exists(partial_c_out)) {
            c_visitor.writer.append_file(partial_c_out.c_str());
        } else {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("missing partial.2c.c, even though module hasn't changed");
#endif
        }

        // this will set all the generic instantiations to generated
        // which means generic decls won't generate those instantiations again
        // it will also set all structs/variants as declared, so they won't be defined twice (in generated c)
        process_cached_module(processor, mod->direct_files, true);
        for(auto& dep : mod->dependencies) {
            process_cached_module(processor, dep.module->direct_files, true);
        }

        // removing non public nodes, because these would be disposed when allocator clears
        remove_non_public_nodes(processor, mod->direct_files);

        // disposing data
        mod_allocator->clear();

        // the module hasn't changed
        return 0;

    }

    if(verbose) {
        std::cout << "[lab] " << "compiling module files" << std::endl;
    }

    // the starting point where this module started translating
    const auto start = c_visitor.writer.current_pos_data();

    // compile the whole module
    processor.translate_module(
            c_visitor, mod
    );

    if(caching) {
        // saving all the c this module wrote in a partial file (for caching)
        auto view = std::string_view(start, c_visitor.writer.current_pos_data() - start);
        auto partial_c_out = get_partial_c_path(build_dir, mod);
        writeToFile(partial_c_out, view);
        // save a mod timestamp
        save_mod_timestamp(direct_files, get_mod_timestamp_path(build_dir, mod, true), options->out_mode);
    }

    if(verbose) {
        std::cout << "[lab] " << "disposing non-public symbols in the module" << std::endl;
    }

    // removing non public nodes, because these would be disposed when allocator clears
    remove_non_public_nodes(processor, mod->direct_files);

    // disposing data
    mod_allocator->clear();

    return 0;

}

#ifdef COMPILER_BUILD

int LabBuildCompiler::process_module_gen_bm(
        LabModule* mod,
        ASTProcessor& processor,
        Codegen& gen,
        CTranslator& cTranslator,
        const std::string_view& build_dir
) {

    const auto bm_mod = options->benchmark_modules;

    // benchmark for the module compilation
    BenchmarkResults bm;

    if(bm_mod) {
        bm.benchmark_begin();
    }

    // the actual translation happens here
    const auto result = process_module_gen(mod, processor, gen, cTranslator, build_dir);
    if(result != 0) {
        return result;
    }

    // end and print the benchmark for module
    if(bm_mod) {
        bm.benchmark_end();
        // printing the benchmark
        ASTProcessor::print_benchmarks(std::cout, "bm:module", mod->format(), &bm);
    }

    return 0;

}

int LabBuildCompiler::process_module_gen(
        LabModule* mod,
        ASTProcessor& processor,
        Codegen& gen,
        CTranslator& cTranslator,
        const std::string_view& build_dir
) {

    // variables
    const auto caching = options->is_caching_enabled;
    const auto verbose = options->verbose;
    const bool is_use_obj_format = options->use_mod_obj_format;
    auto& resolver = *processor.resolver;
    auto& direct_files = mod->direct_files;

    // this would import these direct files (lex and parse), into the module files
    // the module files will have imports, any file imported (from this module or external module will be included)
    const auto parse_success = processor.import_module_files_direct(pool, direct_files, mod);

    // return failure if parse failed
    if(!parse_success) {
        return 1;
    }

    auto& mod_data_path = is_use_obj_format ? mod->object_path : mod->bitcode_path;
    if(!mod_data_path.empty()) {
        std::cout << rang::bg::gray << rang::fg::black << "[lab] " << "Building module ";
        if (!mod->name.empty()) {
            std::cout << '\'' << mod->name.data() << "' ";
        }
        std::cout << "at path '" << mod_data_path << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
    }

    // let c translator know that a new module has begin
    // so it can re-declare imported c headers
    cTranslator.module_begin();

    // prepare for code generation of this module
    gen.module_init(mod->scope_name.to_chem_view(), mod->name.to_chem_view(), &mod->module_scope);

    // start a module scope in symbol resolver, that we can dispose later
    resolver.module_scope_start();

    // importing c headers of the module before processing files
    if(!mod->headers.empty()) {
        if(verbose) {
            std::cout << "[lab] " << "including c headers for module" << std::endl;
        }
        // args to clang
        std::vector<std::string> args;
        args.emplace_back(options->exe_path);
        for(auto& header: mod->headers) {
            args.emplace_back("-include");
            args.emplace_back(header.to_view());
        }
        args.emplace_back("-x");
        args.emplace_back("c");
#ifdef WIN32
        args.emplace_back("NUL");
#else
        args.emplace_back("/dev/null");
#endif
        // set checking for declarations to false, since this is a new module (so none have been included, we are also using a single invocation)
        // this will improve performance (since it hashes their locations), and reliability (since locations sometimes can't be found)
        auto prev_check = cTranslator.check_decls_across_invocations;
        cTranslator.check_decls_across_invocations = false;
        // translate
        cTranslator.translate(args, options->resources_path.c_str());
        cTranslator.check_decls_across_invocations = prev_check;
        auto& nodes = cTranslator.nodes;
        // symbol resolving c nodes, really fast -- just declaring their id's as less than public specifier
        resolver.import_file(nodes, mod->name.to_std_string() + ":headers", true);
        // declaring the nodes fast using code generator
        for(const auto node : nodes) {
            node->code_gen_declare(gen);
        }
    }

    if(verbose) {
        std::cout << "[lab] " << "resolving symbols in the module" << std::endl;
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = processor.sym_res_module(mod);
    if(sym_res_status != 0) {
        return 1;
    }

    // type verify the module
    if(!processor.type_verify_module_parallel(pool, mod)) {
        return 1;
    }

    // check if module has not changed, and use cache appropriately
    // not changed means object file is also present (currently
    if(mod->has_changed.has_value() && !mod->has_changed.value()) {

        if(verbose) {
            std::cout << "[lab] " << "module hasn't changed, processing cached module" << std::endl;
        }

        // this will set all the generic instantiations to generated
        // which means generic decls won't generate those instantiations
        process_cached_module(processor, mod->direct_files, false);
        for(auto& dep : mod->dependencies) {
            process_cached_module(processor, dep.module->direct_files, false);
        }

        // removing non public nodes, because these would be disposed when allocator clears
        remove_non_public_nodes(processor, mod->direct_files);

        // disposing data
        mod_allocator->clear();

        // the module hasn't changed
        return 0;

    }


    if(verbose) {
        std::cout << "[lab] " << "compiling module files" << std::endl;
    }

    // compile the whole module
    processor.compile_module(
            gen, mod
    );

    if(verbose) {
        std::cout << "[lab] " << "disposing non-public symbols in the module" << std::endl;
    }

    // removing non public nodes, because these would be disposed when allocator clears
    remove_non_public_nodes(processor, mod->direct_files);

    if(gen.has_errors) {
        std::cerr << rang::fg::red << "couldn't perform job due to errors during code generation" << rang::fg::reset << std::endl;
        return 1;
    }

    // finalizing the di builder
    gen.di.finalize();

    // disposing data
    mod_allocator->clear();

    CodegenEmitterOptions emitter_options;
    // emitter options allow to configure type of build (debug or release)
    // configuring the emitter options
    configure_emitter_opts(options->out_mode, &emitter_options);
    if (options->def_lto_on) {
        emitter_options.lto = true;
    }
    if(options->debug_ir) {
        emitter_options.debug_ir = true;
    }
    if (options->def_assertions_on) {
        emitter_options.assertions_on = true;
    }

    // which files to emit
    if(!mod->llvm_ir_path.empty()) {
        if(options->debug_ir) {
            if(verbose) {
                std::cout << "[lab] saving debug llvm ir at path '" << mod->llvm_ir_path << '\'' << std::endl;
            }
            gen.save_to_ll_file_for_debugging(mod->llvm_ir_path.data());
        } else {
            emitter_options.ir_path = mod->llvm_ir_path.data();
        }
    }
    if(!mod->asm_path.empty()) {
        emitter_options.asm_path = mod->asm_path.data();
    }
    if(!mod->bitcode_path.empty()) {
        emitter_options.bitcode_path = mod->bitcode_path.data();
    }
    if(!mod->object_path.empty()) {
        emitter_options.obj_path = mod->object_path.data();
    }

    auto& gen_path = is_use_obj_format ? mod->object_path : mod->bitcode_path;
    if(verbose) {
        std::cout << "[lab] emitting the module '" << mod->name << "' at '" << gen_path << '\'' << std::endl;
    }

    // creating a object or bitcode file
    const bool save_result = gen.save_with_options(&emitter_options);
    if(save_result) {
        if(caching && !gen_path.empty()) {
            save_mod_timestamp(direct_files, get_mod_timestamp_path(build_dir, mod, false), options->out_mode);
        }
    } else {
        std::cerr << "[lab] failed to emit file '" << gen_path << '\'' << std::endl;
        return 1;
    }

    return 0;

}

#endif

void begin_job_print(LabJob* job) {

    std::cout << rang::bg::blue << rang::fg::black << "[lab] ";
    switch(job->type) {
        case LabJobType::Executable:
            std::cout << "Building executable";
            break;
        case LabJobType::JITExecutable:
            std::cout << "Building JIT executable";
            break;
        case LabJobType::Library:
            std::cout << "Building library";
            break;
        case LabJobType::ToCTranslation:
            std::cout << "Translating to c";
            break;
        case LabJobType::ToChemicalTranslation:
            std::cout << "Translating to chemical";
            break;
        case LabJobType::Intermediate:
            std::cout << "Building intermediate files";
            break;
        case LabJobType::ProcessingOnly:
            std::cout << "Building objects";
            break;
        case LabJobType::CBI:
            std::cout << "Building CBI";
            break;
    }
    if(!job->name.empty()) {
        std::cout << ' ' << '\'' << job->name.data() << '\'';
    }
    if(!job->abs_path.empty()) {
        std::cout << " at path '" << job->abs_path.data() << '\'';
    }
    std::cout << std::endl << rang::bg::reset << rang::fg::reset;

}

void create_or_rebind_container(LabBuildCompiler* compiler, GlobalInterpretScope& global, SymbolResolver& resolver, const TargetData& data) {
    const auto verbose = compiler->options->verbose;
    if(compiler->container) {
        if(verbose) {
            std::cout << "[lab] " << "rebinding comptime methods" << std::endl;
        }
        // bind global container that contains namespaces like std and compiler
        // reusing it, if we created it before
        global.rebind_container(resolver, compiler->container, data);
    } else {
        if(verbose) {
            std::cout << "[lab] " << "creating comptime methods" << std::endl;
        }
        // allow user the compiler (namespace) functions in @comptime
        // we create the new global container here once
        compiler->container = global.create_container(resolver, data);
        if(verbose) {
            std::cout << "[lab] " << "created the global container" << std::endl;
        }
    }
}

int compile_c_or_cpp_module(LabBuildCompiler* compiler, LabModule* mod, const std::string& mod_timestamp_file) {
#ifndef COMPILER_BUILD
    if(mod->type == LabModuleType::CPPFile) {
        std::cerr << "[lab] " << rang::fg::yellow << "warning: " << rang::fg::reset << "skipping compilation of c++ module '" << *mod << '\'' << std::endl;
        return 2;
    }
#endif
    const auto is_use_obj_format = compiler->options->use_mod_obj_format;
    const auto caching = compiler->options->is_caching_enabled;
    if(mod->paths.empty()) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "non-existent path in a c/c++ module '" << *mod << '\'' << std::endl;
        return 1;
    }
    std::cout << rang::bg::gray << rang::fg::black << "[lab] ";
    if(mod->type == LabModuleType::CFile) {
        std::cout << "compiling c ";
    } else {
        std::cout << "compiling c++ ";
    }
    if (!mod->name.empty()) {
        std::cout << '\'' << mod->name.data() << "' ";
    }
    auto& gen_path = is_use_obj_format ? mod->object_path : mod->bitcode_path;
    std::cout << "at path '" << gen_path << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
#ifdef COMPILER_BUILD
    const auto compile_result = compile_c_file_to_object(mod->paths[0].to_view(), gen_path.to_view(), compiler->options->exe_path, compiler->options->resources_path);
    if (compile_result != 0) {
        return compile_result;
    }
#else
    if(mod->type == LabModuleType::CPPFile) {
        std::cerr << rang::fg::yellow << "[lab] skipping compilation of C++ module '" << *mod << '\'' << rang::fg::reset << std::endl;
        return 1;
    }
    const auto compile_result = compile_adding_file(compiler->options->exe_path.data(), mod->paths[0].data(), mod->object_path.to_std_string(), false, false, to_tcc_mode(compiler->options));
    if (compile_result != 0) {
        return compile_result;
    }
#endif
    if(caching) {
        std::vector<std::string_view> paths;
        for (auto& path: mod->paths) {
            paths.emplace_back(path.to_view());
        }
        save_mod_timestamp(paths, mod_timestamp_file, compiler->options->out_mode);
    }
    return 0;
}

void create_mod_dir(LabBuildCompiler* compiler, LabJobType job_type, const std::string_view& build_dir, LabModule* mod) {
    const auto verbose = compiler->options->verbose;
    const auto use_tcc = compiler->use_tcc(job_type);
    const auto is_use_obj_format = use_tcc || compiler->options->use_mod_obj_format;
    // creating the module directory
    auto module_dir_path = resolve_rel_child_path_str(build_dir, mod->format('.'));
    auto mod_obj_path = resolve_rel_child_path_str(module_dir_path, (use_tcc ? "object_tcc.o" : (is_use_obj_format ? "object.o" : "object.bc")));
    if (!module_dir_path.empty() && job_type != LabJobType::ToCTranslation) {
        if (!exists_with_error(module_dir_path)) {
            if (verbose) {
                std::cout << "[lab] " << "creating module directory at path '" << module_dir_path << "'" << std::endl;
            }
            create_dir_no_check(module_dir_path);
        }
    }
    switch(job_type) {
        case LabJobType::Executable:
        case LabJobType::JITExecutable:
        case LabJobType::CBI:
        case LabJobType::ProcessingOnly:
        case LabJobType::Intermediate:
        case LabJobType::Library:
            if (is_use_obj_format) {
                mod->object_path.clear();
                mod->object_path.append(mod_obj_path);
            } else {
                mod->bitcode_path.clear();
                mod->bitcode_path.append(mod_obj_path);
            }
        default:
            break;
    }
}

void create_job_build_dir(bool verbose, const std::string& build_dir) {
    if(!build_dir.empty()) {
        if(verbose) {
            std::cout << "[lab] " << "creating build directory at '" << build_dir << "'" << std::endl;
        }
        create_dir(build_dir);
    }
}

const char* to_string(OutputMode mode) {
    switch(mode) {
        case OutputMode::Debug:
        default:
            return "debug";
        case OutputMode::DebugQuick:
            return "debug_quick";
        case OutputMode::DebugComplete:
            return "debug_complete";
        case OutputMode::ReleaseFast:
            return "release_fast";
        case OutputMode::ReleaseSmall:
            return "release_small";
        case OutputMode::ReleaseSafe:
            return "release_safe";
    }
}

int LabBuildCompiler::tcc_run_invocation(
    char* exe_path,
    std::vector<std::string_view>& obj_files,
    OutputMode mode,
    int argc,
    char** argv
) {

    const auto state = setup_tcc_state(exe_path, "", true, to_tcc_mode(mode, false));
    if(state == nullptr) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't create tcc state for jit" << std::endl;
        return 1;
    }

    // add object files to link
    for(const auto& dep : obj_files) {
        if(tcc_add_file(state, dep.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to add object file '" << dep <<  "'" << std::endl;
            tcc_delete(state);
            return 1;
        }
    }

    // prepare for jit
    prepare_tcc_state_for_jit(state);

    // run the main method
    const auto status = tcc_run(state, argc, argv);

    // delete the tcc state
    tcc_delete(state);

    // return the status
    return status;

}

// Replaceable limit for maximum argv entries (exe + flags + linkables + sentinel)
constexpr size_t MAX_ARGS = 256;

int LabBuildCompiler::launch_tcc_jit_exe(LabJob* job, std::vector<LabModule*>& dependencies) {

    auto& compiler_exe_path = options->exe_path;

    // Build argv on stack (no heap allocations)
    // argv will contain pointers to NUL-terminated strings that must outlive exec/spawn call (they are on stack)
    // Use mutable char* array because spawn/exec signatures are not const-correct.
    char* argv[MAX_ARGS];
    size_t argi = 0;

    if (argi + 4 >= MAX_ARGS) return -1; // safety; should never happen here

    // exe_path.c_str() is valid; cast away const for exec/spawn API
    argv[argi++] = const_cast<char*>(compiler_exe_path.c_str());

    // mode flag and its numeric string stored in stack buffer
    argv[argi++] = const_cast<char*>("--mode");

    // output mode argument
    char modebuf[64];
    int written = snprintf(modebuf, sizeof(modebuf), "%s", to_string(options->out_mode));
    if (written <= 0) return -2;
    argv[argi++] = modebuf;

    // Add all linkables (these string_views must refer to stable storage)
    for (const auto &sv : job->objects) {
        if (argi + 2 >= MAX_ARGS) return -3; // too many args
        argv[argi++] = const_cast<char*>(sv.data());
    }

    // final literal arg
    argv[argi++] = const_cast<char*>("tcc-jit");

    // sentinel
    argv[argi] = nullptr;

    // TODO: support user arguments
    return launch_process_and_wait(compiler_exe_path.c_str(), argv, argi, true);

}

int LabBuildCompiler::link_cbi_job(LabJobCBI* cbiJob, std::vector<LabModule*>& dependencies) {

    auto& job_name = cbiJob->name;
    auto cbiName = cbiJob->name.to_std_string();

    // plugin mode is used, reason its separate is because
    // tiny cc links in some functions when using -b, which is used in debug complete
    // when we use cache, we output object files with -b, now if user changes the mode to release
    // we still use the same cache for plugins and without recompilation, there are link errors
    const auto state = setup_tcc_state(options->exe_path.data(), "", true, to_tcc_mode(options));
    if(state == nullptr) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't create tcc state for jit of cbi '" << job_name << '\'' << std::endl;
        return 1;
    }

    // add object files to link
    for(const auto& dep : cbiJob->objects) {
        if(tcc_add_file(state, dep.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to add object file '" << dep <<  "' in compilation of cbi '" << job_name << '\'' << std::endl;
            tcc_delete(state);
            return 1;
        }
    }

    // prepare for jit
    prepare_tcc_state_for_jit(state);

    // flattened dependencies
    auto& outModDependencies = dependencies;

    // import all compiler interfaces the modules require
    for(const auto mod : outModDependencies) {
        for(auto& interface : mod->compiler_interfaces) {
            CompilerBinder::import_compiler_interface(interface, state);
        }
    }

    // relocate the code before calling
    if(tcc_relocate(state) == -1) {
        std::cerr << "[lab] " << rang::fg::red <<  "error: " << rang::fg::reset << "failed to relocate cbi '" << job_name << '\'' << std::endl;
        tcc_delete(state);
        return 1;
    }

    auto& cbiData = binder.data[cbiName];

    // we compile the entirety of this module and store it
    // here putting this module in cbi is what will delete it
    // this is very important, otherwise tcc_delete won't be called on it
    cbiData.module = state;

    // error out if cbi types are empty
    if(cbiJob->indexes.empty()) {
        std::cerr << "[lab] " << rang::fg::red <<  "error: " << rang::fg::reset << "cbi job has no cbi types '" << job_name << '\'' << std::endl;
        return 1;
    }

    // preparing cbi types
    for(auto& index : cbiJob->indexes) {
        auto err = binder.index_function(index, state);
        if(err != nullptr) {
            std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << err << " when indexing cbi function '" << index.fn_name << "' with key '" << index.key << '\'' << std::endl;
            return 1;
        }
    }

    return 0;

}

int LabBuildCompiler::process_job_tcc(LabJob* job) {


    const auto exe = job;
    const auto get_job_type = job->type;
    const auto job_type = exe->type;

    const auto is_job_cbi = get_job_type == LabJobType::CBI;
    // job caching means relink all if all objects are present
    const auto job_caching = is_job_cbi ? (options->force_recompile_plugins == false) : options->is_caching_enabled;
    const auto verbose = options->verbose;
    const auto bm_mod = options->benchmark_modules;

    begin_job_print(job);

    // configure output path
    const bool is_use_obj_format = options->use_mod_obj_format;

    if(verbose) {
        std::cout << "[lab] " << "flattening the module structure" << std::endl;
    }

    // process remote imports
    // TODO: new context being created to handle remote imports
    LabBuildContext context(*this, path_handler, mod_storage, binder);
    auto ri_res = process_remote_imports(context, job);
    if(ri_res != 0) return ri_res;

    // flatten the dependencies
    auto dependencies = flatten_dedupe_sorted(exe->dependencies);

    // zero out cached children, so they are calculated again during sym res
    zero_out_cached_children(dependencies);

    // allocating required variables before going into loop
    bool do_compile = job_type != LabJobType::ToCTranslation;

    // build dir
    auto build_dir = exe->build_dir.to_std_string();

    // creating the job build directory
    create_job_build_dir(verbose, build_dir);

    // we put all modules of a job in this directory
    auto mods_dir = resolve_rel_child_path_str(build_dir, "modules");

    // creating the modules direcotry
    create_dir(mods_dir);

    // outputting an object file
    auto job_obj_path = resolve_rel_child_path_str(build_dir, "object_tcc.o");

    // for each module, let's determine its files
    for (const auto mod: dependencies) {

        // determining module's direct files
        ASTProcessor::determine_module_files(path_handler, loc_man, mod);

        // we must recalculate whether module's files have changed
        mod->has_changed = std::nullopt;

        // creating the module directory and getting the timestamp file path
        create_mod_dir(this, exe->type, mods_dir, mod);

    }

    // if caching is enabled, we check if none of the files have changed
    if(job_caching && do_compile) {

        // if not a single module has changed, we consider it true
        bool has_any_changed = false;

        // check the job object path exists (since we are caching)
        if (!fs::exists(job_obj_path)) {

            // checking which modules have changed
            for (auto& dep: exe->dependencies) {
                auto has_changed = has_module_changed(this, dep.module, mods_dir, true);
                if (has_changed) {
                    has_any_changed = true;
                }
            }

        }

        if (!has_any_changed) {

            // NOTE: there exists not a single module (or file) that has changed
            // which means we can safely link the previous job object file again
            // we need to return early so modules won't be parsed at all
            if(verbose) {
                std::cout << "[lab] " << "successfully reusing job object file at '" << job_obj_path << "'" << std::endl;
            }

            // job object file that is already present
            job->objects.emplace_back(job_obj_path);
            // including any object files from C file modules
            for(const auto mod : dependencies) {
                if(mod->type == LabModuleType::CFile) {
                    job->objects.emplace_back(mod->object_path.to_chem_view());
                }
            }

            // for cbi/jit jobs, we need to link and run them
            if (get_job_type == LabJobType::CBI) {
                const auto cbiJob = (LabJobCBI*) job;
                const auto jobDone = link_cbi_job(cbiJob, dependencies);
                if (jobDone != 0) {
                    return jobDone;
                }
            } else if (get_job_type == LabJobType::JITExecutable) {
                const auto status = launch_tcc_jit_exe(job, dependencies);
                if (status != 0) {
                    return status;
                }
            }

            // cached job
            return 0;

        }

    }

    if(verbose) {
        std::cout << "[lab] " << "allocating instances objects required for building" << std::endl;
    }

    // job caching avoided
    // an interpretation scope for interpreting compile time function calls
    GlobalInterpretScope global(job->mode, job->target_data, nullptr, this, *job_allocator, type_builder, loc_man);

    // we hold the instantiated types inside this container
    InstantiationsContainer instContainer;

    // a new symbol resolver for every executable
    SymbolResolver resolver(binder, global, path_handler, controller, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // beginning
    ToCAstVisitor c_visitor(binder, global, mangler, *file_allocator, loc_man, options->debug_info, options->minify_c);
    ToCBackendContext c_context(&c_visitor);
    global.backend_context = (BackendContext*) &c_context;

    // the processor we use
    ASTProcessor processor(path_handler, options, mod_storage, controller, loc_man, &resolver, binder, type_builder, *job_allocator, *mod_allocator, *file_allocator);

    // create or rebind the global container (comptime functions like intrinsics namespace)
    create_or_rebind_container(this, global, resolver, job->target_data);

    // begin translation
    c_visitor.prepare_translate();

    // if user only asked us to compile c files, we must not link the chemical object file
    auto did_compile_chemical = false;

    // compile dependencies modules for this executable
    for(auto mod : dependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing module " << *mod << std::endl;
        }

        if(do_compile) {
            switch (mod->type) {
                case LabModuleType::CPPFile:
                case LabModuleType::CFile: {
                    if(!mod->has_changed.has_value() || mod->has_changed.value()) {
                        const auto c_res = compile_c_or_cpp_module(this, mod, get_mod_timestamp_path(mods_dir, mod, true));
                        if (c_res == 0) {
                            job->objects.emplace_back(mod->object_path.copy());
                            continue;
                        } else if(c_res == 2) {
                            continue;
                        } else {
                            return 1;
                        }
                    } else {
                        job->objects.emplace_back(mod->object_path.copy());
                        continue;
                    }
                }
                case LabModuleType::ObjFile:
                    for(auto& path : mod->paths) {
                        exe->objects.emplace_back(path.copy());
                    }
                    continue;
                default:
                    break;
            }
        } else if (mod->type == LabModuleType::ObjFile
#ifdef COMPILER_BUILD
                || mod->type == LabModuleType::CFile
#endif
        ) {
            continue;
        }

        // the actual translation happens here
        const auto result = process_module_tcc_bm(mod, processor, c_visitor, mods_dir);
        if(result != 0) {
            return result;
        }

        if(!did_compile_chemical) {
            did_compile_chemical = true;
        }

    }

    // add the job obj path to linkables
    if(did_compile_chemical) {
        job->objects.emplace_back(job_obj_path);
    }

    // end the translation (must be done)
    c_visitor.end_translate();

    auto program = c_visitor.writer.finalized_std_view();

    if(job_type == LabJobType::ToCTranslation) {
        // skip compilation, only c translation required
        writeToFile(job->abs_path.to_std_string(), program);
        return 0;
    }

    const auto intermediate_job = job_type == LabJobType::Intermediate;
    const auto emit_c = intermediate_job || options->emit_c;

    if(emit_c) {
        writeToFile(resolve_rel_child_path_str(build_dir, "Translated.c"), program);
    }

    if(intermediate_job) {
        // skip compilation, only intermediates required
        return 0;
    }

    // compiling the entire C to a single object file
    const auto compile_c_result = compile_c_string(options->exe_path.data(), program.data(), job_obj_path, false, options->benchmark, to_tcc_mode(options));
    if (compile_c_result != 0) {
        auto out_path = resolve_rel_child_path_str(build_dir, "2c.debug.c");
        writeToFile(out_path, program);
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't build c program due to error in translation, written at " << out_path << std::endl;
        return compile_c_result;
    }

    // cbi and jit jobs are here
    if(get_job_type == LabJobType::CBI) {
        const auto cbiJob = (LabJobCBI*) job;
        const auto jobDone = link_cbi_job(cbiJob, dependencies);
        if(jobDone != 0) {
            return jobDone;
        }
    } else if(get_job_type == LabJobType::JITExecutable) {
        const auto status = launch_tcc_jit_exe(job, dependencies);
        if(status != 0) {
            return status;
        }
    }

    return 0;

}

#ifdef COMPILER_BUILD

int LabBuildCompiler::process_job_gen(LabJob* job) {

    const auto job_type = job->type;

    const auto is_job_cbi = job_type == LabJobType::CBI;
    // job caching means relink all if all objects are present
    const auto job_caching = is_job_cbi ? (options->force_recompile_plugins == false) : options->is_caching_enabled;
    const auto verbose = options->verbose;

    begin_job_print(job);

    // configure output path
    const bool is_use_obj_format = options->use_mod_obj_format;

    if(verbose) {
        std::cout << "[lab] " << "flattening the module structure" << std::endl;
    }

    // process remote imports
    // TODO: new context being created to handle remote imports
    LabBuildContext context(*this, path_handler, mod_storage, binder);
    auto ri_res = process_remote_imports(context, job);
    if(ri_res != 0) return ri_res;

    // flatten the dependencies
    auto dependencies = flatten_dedupe_sorted(job->dependencies);

    // zero out cached children, so they are calculated again during sym res
    zero_out_cached_children(dependencies);

    // build dir
    auto build_dir = job->build_dir.to_std_string();

    // creating the job build directory
    create_job_build_dir(verbose, build_dir);

    // we put all modules of a job in this directory
    auto mods_dir = resolve_rel_child_path_str(build_dir, "modules");

    // creating the modules direcotry
    create_dir(mods_dir);

    // for each module, let's determine its files and whether it has changed
    for(const auto mod : dependencies) {

        // determining module's direct files
        ASTProcessor::determine_module_files(path_handler, loc_man, mod);

        // we must recalculate which files have changed
        mod->has_changed = std::nullopt;

        // creating the module directory and getting the timestamp file path
        create_mod_dir(this, job->type, mods_dir, mod);

    }

    if(job_caching) {

        // if not a single module has changed, we consider it true
        bool has_any_changed = false;

        for(auto& dep : job->dependencies) {
            const auto changed = has_module_changed(this, dep.module, mods_dir, false);
            if(changed) {
                has_any_changed = true;
            }
        }

        if(!has_any_changed) {

            // NOTE: there exists not a single module that has changed
            // which means we can safely link the previous object files again
            // we need to return early so modules won't be parsed at all

            for (const auto mod: dependencies) {
                job->objects.emplace_back(mod->object_path.to_chem_view());
            }

            return 0;
        }

    }

    if(verbose) {
        std::cout << "[lab] " << "allocating instances objects required for building" << std::endl;
    }

    // an interpretation scope for interpreting compile time function calls
    GlobalInterpretScope global(job->mode, job->target_data, nullptr, this, *job_allocator, type_builder, loc_man);

    // generic instantiations types are stored here
    InstantiationsContainer instContainer;

    // a new symbol resolver for every executable
    SymbolResolver resolver(binder, global, path_handler, controller, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    auto& job_alloc = *job_allocator;
    // a single c translator across this entire job
    CTranslator cTranslator(job_alloc, type_builder, options->is64Bit);
    ASTProcessor processor(path_handler, options, mod_storage, controller, loc_man, &resolver, binder, type_builder, job_alloc, *mod_allocator, *file_allocator);
    CodegenOptions code_gen_options;
    if(cmd) {
        code_gen_options.fno_unwind_tables = cmd->has_value("", "fno-unwind-tables");
        code_gen_options.fno_asynchronous_unwind_tables = cmd->has_value("", "fno-asynchronous-unwind-tables");
        code_gen_options.no_pie = cmd->has_value("no-pie", "no-pie");
    }
    Codegen gen(code_gen_options, binder, global, mangler, job->target_triple.to_std_string(), options->exe_path, options->is64Bit, options->debug_info, *file_allocator);
    LLVMBackendContext g_context(&gen);
    // set the context so compile time calls are sent to it
    global.backend_context = (BackendContext*) &g_context;

    create_or_rebind_container(this, global, resolver, job->target_data);

    // before compilation of the module, we prepare the target machine
    const auto machine = gen.setup_for_target(gen.target_triple, is_debug(options->out_mode));
    if(machine == nullptr) {
        std::cerr << "[lab] " << "failure to create target machine" << std::endl;
        return 1;
    }

    // compile dependent modules for this executable
    for(auto mod : dependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing module '" << mod->name << '\'' << std::endl;
        }

        // handle c and cpp file modules
        switch (mod->type) {
            case LabModuleType::CFile:
            case LabModuleType::CPPFile: {
                if(!mod->has_changed.has_value() || mod->has_changed.value()) {
                    const auto c_res = compile_c_or_cpp_module(this, mod, get_mod_timestamp_path(mods_dir, mod, false));
                    if(c_res == 0) {
                        if(is_use_obj_format) {
                            job->objects.emplace_back(mod->object_path.copy());
                        } else {
                            job->objects.emplace_back(mod->bitcode_path.copy());
                        }
                        continue;
                    } else {
                        return 1;
                    }
                } else {
                    job->objects.emplace_back(mod->object_path.copy());
                    continue;
                }
            }
            case LabModuleType::ObjFile:
                job->objects.emplace_back(mod->paths[0].copy());
                continue;
            default:
                break;
        }

        if(job_type == LabJobType::Intermediate) {
            // prevent generation of object / bitcode file
            mod->object_path.clear();
            mod->bitcode_path.clear();
        }

        const auto result = process_module_gen_bm(mod, processor, gen, cTranslator, mods_dir);
        if(result != 0) {
            return result;
        }

        if(is_use_obj_format) {
            job->objects.emplace_back(mod->object_path.copy());
        } else {
            job->objects.emplace_back(mod->bitcode_path.copy());
        }

    }

    return 0;

}

#endif

void print_failed_to_link(std::vector<chem::string>& linkables, const std::string& output_path) {
    std::cerr << "Failed to link \n";
    for(auto& linkable : linkables) {
        std::cerr << '\t' << linkable << '\n';
    }
    std::cerr << "into " << output_path;
    std::cerr << std::endl;
}

int link_objects_tcc(
        const std::string& comp_exe_path,
        std::vector<chem::string>& objects,
        std::vector<chem::string>& link_libs,
        const std::string& output_path,
        TCCMode mode
) {
    chem::string copy(comp_exe_path);
    const auto link_result = tcc_link_objects(copy.mutable_data(), output_path, objects, link_libs, mode);
    if(link_result != 0) {
        print_failed_to_link(objects, output_path);
    }
    return link_result;
}

int link_objects_now(
    bool use_tcc,
    LabBuildCompilerOptions* options,
    std::vector<chem::string>& objects,
    std::vector<chem::string>& link_libs,
    const std::string& output_path,
    const std::string_view& target_triple
) {
    if(options->verbose) {
        std::cout << "[lab] linking objects ";
        for(auto& obj : objects) {
            std::cout << '\'' << obj.to_view() << '\'' << ' ';
        }
        std::cout << "into '" << output_path << '\'';
        std::cout << std::endl;
    }
#ifdef COMPILER_BUILD
    if(use_tcc) {
        return link_objects_tcc(options->exe_path, objects, link_libs, output_path, to_tcc_mode(options->out_mode, options->debug_info));
    } else {
        LinkFlags linkFlags;
        linkFlags.debug_info = options->debug_info || is_debug_or_compl(options->out_mode);
        linkFlags.verbose = options->verbose_link;
        linkFlags.no_pie = options->no_pie;
        int link_result;
        if(options->use_lld) {
            link_result = lld_link_objects(objects, output_path, options->exe_path, link_libs, target_triple, linkFlags);
        } else {
            link_result = clang_link_objects(objects, output_path, options->exe_path, link_libs, target_triple, linkFlags, options->resources_path);
        }
        if(link_result != 0) {
            print_failed_to_link(objects, output_path);
        }
        return link_result;
    }
#else
    return link_objects_tcc(options->exe_path, objects, link_libs, output_path, to_tcc_mode(options->out_mode, options->debug_info));
#endif
}

int LabBuildCompiler::do_executable_job(LabJob* job) {
    auto result = process_modules(job);
    if(result != 0) {
        return result;
    }
    // link will automatically detect the extension at the end
    return link_objects_now(use_tcc(job), options, job->objects, job->link_libs, job->abs_path.to_std_string(), job->target_triple.to_view());
}

int LabBuildCompiler::do_library_job(LabJob* job) {
    auto result = process_modules(job);
    if(result != 0) {
        return result;
    }
    // link will automatically detect the extension at the end
    return link_objects_now(use_tcc(job), options, job->objects, job->link_libs, job->abs_path.to_std_string(), job->target_triple.to_view());
}

int LabBuildCompiler::do_to_chemical_job(LabJob* job) {
#ifdef COMPILER_BUILD
    std::ofstream output;
    output.open(job->abs_path.data());
    if(!output.is_open()) {
        std::cerr << rang::fg::red << "[lab] " << "couldn't open the file for writing translated chemical from c at '" << job->abs_path.data() << '\'' << rang::fg::reset << std::endl;
        return 1;
    }
    for(auto& dep : job->dependencies) {
        const auto mod = dep.module;
        RepresentationVisitor visitor(output);
        std::vector<std::string> args;
        args.emplace_back(options->exe_path);
        for(auto& header : mod->headers) {
            args.emplace_back("-include");
            args.emplace_back(header.to_view());
        }
        if(mod->paths.empty()) {
            args.emplace_back("-x");
            args.emplace_back("c");
#ifdef WIN32
            args.emplace_back("NUL");
#else
            args.emplace_back("/dev/null");
#endif
        } else {
            for (auto& path: mod->paths) {
                args.emplace_back(path.to_view());
            }
        }
        // the c translator we will use
        CTranslator cTranslator(*mod_allocator, type_builder, options->is64Bit);
        // we will only do a single invocation
        cTranslator.check_decls_across_invocations = false;
        cTranslator.translate(args, options->get_resources_path().c_str());
        // get the nodes
        auto& nodes = cTranslator.nodes;
        visitor.translate(nodes);
    }
    output.close();
    return 0;
#else
    std::cerr << "Translating c files to chemical can only be done by the compiler executable, please check using intrinsics::is_clang() in build.lab" << std::endl;
    return 1;
#endif
}

LabModule* LabBuildCompiler::create_module_for_dependency(
        LabBuildContext& context,
        ModuleDependencyRecord& dependency,
        LabJob* job
) {
    auto& module_path = dependency.module_dir_path;
    auto buildLabPath = resolve_rel_child_path_str(module_path, "build.lab");
    if(std::filesystem::exists(buildLabPath)) {

        // check if we have already parsed this build.lab (from another module's dependency)
        {
            std::lock_guard<std::mutex> lock(buildLabDependenciesCacheMutex);
            auto found = buildLabDependenciesCache.find(buildLabPath);
            if(found != buildLabDependenciesCache.end()) {
                return found->second;
            }
        }

        // build lab file into a tcc state
        // TODO verify the build method signature in the build.lab file
        const auto state = built_lab_file(context, buildLabPath, false);

        // emit a warning or error
        if(state == nullptr) {
            return nullptr;
        }

        // automatic destroy
        TCCDeletor auto_del(state);

        // get the build method
        auto build = (LabModule*(*)(LabBuildContext*, LabJob*)) tcc_get_symbol(state, "chemical_lab_build");
        if(!build) {
            std::cerr << rang::fg::red << "[lab] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
            return nullptr;
        }

        // call the root build.lab build's function
        const auto modPtr = build(&context, job);

        if (modPtr) {
            // store the mod pointer in cache, so we don't need to build this build.lab again
            std::lock_guard<std::mutex> lock(buildLabDependenciesCacheMutex);
            buildLabDependenciesCache[std::move(buildLabPath)] = modPtr;
        }

        return modPtr;

    } else {

        auto modFilePath = resolve_rel_child_path_str(module_path, "chemical.mod");
        if(std::filesystem::exists(modFilePath)) {

            // check if we have already parsed this chemical.mod (from another module's dependency)
            {
                std::lock_guard<std::mutex> lock(buildLabDependenciesCacheMutex);
                auto found = buildLabDependenciesCache.find(modFilePath);
                if(found != buildLabDependenciesCache.end()) {
                    return found->second;
                }
            }

            // build the mod file
            const auto modPtr = build_module_from_mod_file(context, modFilePath, job);

            if (modPtr) {
                // store the module pointer in cache
                std::lock_guard<std::mutex> lock(buildLabDependenciesCacheMutex);
                buildLabDependenciesCache[std::move(modFilePath)] = modPtr;
            }

            return modPtr;

        } else {

            std::cerr << rang::fg::red << "error:" << rang::fg::reset << " directory at path '" << module_path << "' doesn't contain a 'build.lab' or 'chemical.mod' therefore cannot be imported" << std::endl;
            return nullptr;

        }


    }

}

int LabBuildCompiler::translate_mod_file_to_lab(
        const chem::string_view& modFilePath,
        const chem::string_view& outputPath
) {

    // lets construct variables that are being used
    LocationManager loc_man;
    ASTAllocator allocator(10000); // using 10kb batches

    // determining the scope and module name from the .mod file
    const auto each_buf_size = 80;
    char temp_scope_name[each_buf_size];
    char temp_module_name[each_buf_size];
    size_t scope_name_size = 0;
    size_t mod_name_size = 0;
    const auto errorMsg = parseModDecl(temp_scope_name, temp_module_name, scope_name_size, mod_name_size, each_buf_size, modFilePath.view());

    // determining the module scope name and module name
    chem::string_view scope_name(temp_scope_name, scope_name_size);
    chem::string_view module_name(temp_module_name, mod_name_size);

    // module file data
    auto filePathStr__ = modFilePath.str(); // two underscores at the end so hard to access (moved)
    auto modFileId = loc_man.encodeFile(filePathStr__);
    ASTFileMetaData meta(modFileId, nullptr, std::move(filePathStr__));
    ModuleFileData modFileData(meta);

    // set those module names
    modFileData.scope_name = scope_name;
    modFileData.module_name = module_name;

    // import the file into result (lex and parse)
    const auto isModFileOk = ASTProcessor::import_chemical_mod_file(allocator, allocator, loc_man, modFileData, modFileId, modFilePath.view());

    // print the result
    ASTDiagnoser::print_diagnostics(modFileData.diagnostics, modFilePath, "Parser");

    // check
    if(!isModFileOk) {
        return 1;
    }

    // opening the file
    std::ofstream stream;
    stream.open(outputPath.data());
    if(!stream.is_open()) {
        std::cerr << rang::fg::red << "error:" << rang::fg::reset << ' ' << "when opening file '" << outputPath << '\'' << std::endl;
        return 1;
    }

    // actual conversion
    convertToBuildLab(modFileData, stream);

    // closing the writing stream
    stream.close();
    return 0;


}

inline static void error_out(LocationManager& loc_man, SourceLocation loc, const chem::string_view& message) {
    std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << message << " at " << loc_man.formatLocation(loc) << std::endl;
}

LabModule* LabBuildCompiler::build_module_from_mod_file(
        LabBuildContext& context,
        const std::string_view& modFilePathView,
        LabJob* job
) {

    const auto verbose = options->verbose;

    if(verbose) {
        std::cout << "[lab] " << "getting the module declaration from '" << modFilePathView << '\'' << std::endl;
    }

    // determining the scope and module name from the .mod file
    const auto each_buf_size = 80;
    char temp_scope_name[each_buf_size];
    char temp_module_name[each_buf_size];
    size_t scope_name_size = 0;
    size_t mod_name_size = 0;
    const auto errorMsg = parseModDecl(temp_scope_name, temp_module_name, scope_name_size, mod_name_size, each_buf_size, modFilePathView);

    // determining the module scope name and module name
    chem::string_view scope_name(temp_scope_name, scope_name_size);
    chem::string_view module_name(temp_module_name, mod_name_size);

//    if(errorMsg == nullptr) {
//        const auto module = context.storage.find_module(scope_name, module_name);
//        if(module != nullptr) {
//            return module;
//        }
//    } else {
//        std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't get module declaration from the mod file at '" << modFilePathView << "' because of error '" << errorMsg << '\'' << std::endl;
//    }

    if(verbose) {
        std::cout << "[lab] " << "parsing mod file '" << modFilePathView << '\'' << std::endl;
    }

    // module file data
    auto modFilePathChemView = chem::string_view(modFilePathView);
    std::string modFilePath__(modFilePathView); // two underscores at the end so hard to access (moved)
    auto modFileId = loc_man.encodeFile(modFilePath__);
    ASTFileMetaData meta(modFileId, nullptr, std::move(modFilePath__));
    ModuleFileData modFileData(meta);

    // set those module names
    modFileData.scope_name = scope_name;
    modFileData.module_name = module_name;

    // import the file into result (lex and parse)
    const auto isModFileOk = ASTProcessor::import_chemical_mod_file(*file_allocator, *mod_allocator, loc_man, modFileData, modFileId, modFilePathView);

    // printing the diagnostics for the file
    Diagnoser::print_diagnostics(modFileData.diagnostics, modFilePathChemView, "Parser");

    // error out if not ok
    if (!isModFileOk) {
        std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't parse the mod file at '" << modFilePathView << "' due to errors" << std::endl;
        return nullptr;
    }

    // create a new module
    const auto module = context.new_module(scope_name, module_name, modFileData.package_kind);

    // get all the sources
    for(auto& src : modFileData.sources_list) {
        if(src.if_cond == nullptr) {
            module->paths.emplace_back(resolve_sibling(modFilePathView, src.path.view()));
        } else {
            auto resolved = resolve_target_condition(job->target_data, src.if_cond);
            if (resolved.has_value()) {
                if(resolved.value()) {
                    module->paths.emplace_back(resolve_sibling(modFilePathView, src.path.view()));
                }
            } else {
                std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't resolve the if condition in '" << modFilePathView << '\'' << std::endl;
            }
        }
    }

    // importing all compiler interfaces user requested inside the .mod file
    for(auto& interface : modFileData.compiler_interfaces) {
        auto found = binder.interface_maps.find(interface);
        if(found != binder.interface_maps.end()) {
            module->compiler_interfaces.emplace_back(found->second);
#ifdef LSP_BUILD
            module->compiler_interfaces_str.emplace_back(chem::string(interface));
#endif
        } else {
            std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "unknown compiler interface '" << interface << "' in mod file a '" << modFilePathView << "'" << std::endl;
            return nullptr;
        }
    }

    // import any link libraries into the executable user requested for this module
    for(auto& linkLib : modFileData.link_libs) {
        auto resolved = resolve_target_condition(job->target_data, linkLib.if_cond);
        if(resolved.has_value()) {
            if(resolved.value()) {
                switch(linkLib.kind) {
                    case ModFileLinkLibKind::Name: {
                        std::lock_guard<std::mutex> lock(job_mutex);
                        job->link_libs.emplace_back(chem::string(linkLib.name));
                        break;
                    }
                    case ModFileLinkLibKind::File:
                        // TODO: support file link libs
                        std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "file based link libraries aren't supported '" << modFilePathView << '\'' << std::endl;
                        continue;
                }
            }
        } else {
            std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't resolve the if condition in '" << modFilePathView << '\'' << std::endl;
        }
    }

    if (verbose) {
        std::cout << "[lab] " << "created module for '" << module->scope_name << ':' << module->name << "'" << std::endl;
    }

    // based on imports figures out which modules have been imported
    for(const auto node : modFileData.scope.body.nodes) {
        if(node->kind() != ASTNodeKind::ImportStmt) {
            break;
        }
        const auto stmt = node->as_import_stmt_unsafe();
        auto result = path_handler.resolve_mod_dep_import(stmt, job->target_data, modFilePathView);
        if(result.isSkipped()) {
            continue;
        }
        if(!result.error_message.empty()) {
            error_out(loc_man, node->encoded_location(), chem::string_view(result.error_message));
            return nullptr;
        }
        // get the module pointer
        // we must build their build.lab or chemical.mod into a LabModule*
        auto record = ModuleDependencyRecord{std::move(result.directory_path)};
        const auto modDependency = create_module_for_dependency(context, record, job);
        if(modDependency == nullptr) {
            return nullptr;
        }
        stmt->setResult(modDependency);
        module->add_dependency(modDependency);
    }

    // clear the allocators
    mod_allocator->clear();
    file_allocator->clear();

    // return the state
    return module;

}

FunctionDeclaration* find_lab_build_method(const std::string& abs_path, std::vector<ASTNode*>& nodes, bool error = true) {
    for(const auto node : nodes) {
        if(node->kind() == ASTNodeKind::FunctionDecl && node->as_function_unsafe()->name_view() == "build") {
            return node->as_function_unsafe();
        }
    }
    std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset << " no build method found in the lab build file " << abs_path << std::endl;
    return nullptr;
}

TCCState* LabBuildCompiler::built_lab_file(
        LabBuildContext& context,
        const std::string_view& path_view,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        bool mod_file_source,
        LabJob* job
) {

    auto& lab_processor = processor;
    auto& lab_resolver = *processor.resolver;
    const auto verbose = options->verbose;
    const auto caching = options->is_caching_enabled;

    if(verbose) {
        std::cout << "[lab] building lab file at '" << path_view << '\'' << std::endl;
    }

    LabModule chemical_lab_module(LabModuleType::Files, chem::string("chemical"), chem::string("lab"));
    std::string path(path_view);
    auto buildLabFileId = loc_man.encodeFile(path);
    ASTFileMetaData buildLabMetaData(buildLabFileId, &chemical_lab_module.module_scope, path, nullptr);
    ASTFileResult labFileResult(buildLabFileId, path, &chemical_lab_module.module_scope);

    // import the file into result (lex and parse)
    // NOTE: we import these files on job allocator, because a build.lab has dependencies on modules
    // that we need to compile, which will free the module allocator, so if we kept on module allocator
    // we will lose everything after processing dependencies
    if(mod_file_source) {
        lab_processor.import_mod_file_as_lab(buildLabMetaData, labFileResult, true);
    } else {
        lab_processor.import_chemical_file(labFileResult, buildLabMetaData.file_id, buildLabMetaData.abs_path, true);
    }

    // probably an error during parsing
    if(!labFileResult.continue_processing) {
        if(verbose) {
            std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't import the 'build.lab' file at '" << path << "' due to errors" << std::endl;
        }
        return nullptr;
    }

    // figure out direct imported by this build.lab file
    auto& direct_files_in_lab = labFileResult.imports;
    auto direct_imports_success = lab_processor.figure_out_direct_imports(buildLabMetaData, labFileResult.unit.scope.body.nodes, direct_files_in_lab);
    if(!direct_imports_success) {
        return nullptr;
    }

    if(verbose) {
        std::cout << "[lab] figured out direct imports, importing files" << std::endl;
    }

    // if has imports, we import those files as well
    // it's required to build a proper import tree
    if(!direct_files_in_lab.empty()) {
        // create a concurrent parsing state
        ConcurrentParsingState state;
        // NOTE: we import these files on job allocator, because a build.lab has dependencies on modules
        // that we need to compile, which will free the module allocator, so if we kept on module allocator
        // we will lose everything after processing dependencies
        const auto parseSuccess = lab_processor.import_chemical_files_recursive(pool, state, direct_files_in_lab, true);
        // wait for all files to parse
        auto fut = state.all_done_promise.get_future();
        fut.wait();

        // return failure if parse failed
        if(!parseSuccess) {
            if(verbose) {
                std::cout << "[lab] " << "parsing failure in lab file at '" << path_view << '\'' << std::endl;
            }
            return nullptr;
        }

    }

    ASTFileResult* files_to_flatten[] = { &labFileResult };

    if(verbose) {
        std::cout << "[lab] detecting import cycles in build.lab files" << std::endl;
    }

    // check module files for import cycles (direct or indirect)
    ImportCycleCheckResult importCycle { false, loc_man };
    check_imports_for_cycles(importCycle, files_to_flatten);
    if(importCycle.has_cyclic_dependencies) {
        if(verbose) {
            std::cout << "[lab] " << "failure due to cyclic dependencies in '" << path_view << '\'' << std::endl;
        }
        return nullptr;
    }

    // flatten the files to module files (in sorted order of independence)
    auto module_files = flatten(files_to_flatten);

    // we figure out all the files that belong to this build.lab module
    direct_files_in_lab.clear();
    for(const auto f : module_files) {
        f->result = f;
        direct_files_in_lab.emplace_back(*f);
    }

    // copy over the imported files metadata to chemical lab module
    // because its processed by the symbol resolver
    chemical_lab_module.direct_files = direct_files_in_lab;

    // the build lab object file (cached)
    const auto labDir = resolve_rel_child_path_str(options->build_dir, "lab");
    const auto labModDir = resolve_rel_child_path_str(labDir, "chemical.lab");

    // create required directories
    create_dir(labDir);
    create_dir(labModDir);

    // figure out where to store the build lab object and dat file
    const auto buildLabObj = resolve_rel_child_path_str(labModDir, "build.lab.o");
    const auto buildLabTimestamp = resolve_rel_child_path_str(labModDir, "build.lab.dat");

    if(verbose) {
        std::cout << "[lab] processing build files of module dependencies from imports" << std::endl;
    }

    // direct module dependencies (in no valid order)
    auto& mod_dependencies = chemical_lab_module.dependencies;

    // based on imports figures out which modules have been imported
    for(auto& file : direct_files_in_lab) {
        for(const auto node : file.result->unit.scope.body.nodes) {
            if(node->kind() != ASTNodeKind::ImportStmt) {
                break;
            }
            const auto stmt = node->as_import_stmt_unsafe();
            auto result = path_handler.resolve_mod_dep_import(stmt, job->target_data, path_view);
            if(result.isSkipped()) {
                continue;
            }
            if(!result.error_message.empty()) {
                error_out(loc_man, node->encoded_location(), chem::string_view(result.error_message));
                return nullptr;
            }
            // get the module pointer
            // we must build their build.lab or chemical.mod into a LabModule*
            auto record = ModuleDependencyRecord{std::move(result.directory_path)};
            const auto mod = create_module_for_dependency(context, record, job);
            if(mod == nullptr) {
                return nullptr;
            }
            stmt->setResult(mod);
            mod_dependencies.emplace_back(mod);
        }
    }

    if(verbose) {
        std::cout << "[lab] determining files that belong to each dependency module" << std::endl;
    }

    // including all (+nested) dependencies in a single vector
    // sorted in the order of least dependence (flattened with all the dependencies in one vector)
    auto outModDependencies = flatten_dedupe_sorted(mod_dependencies);

    // zero out cached children, so they are calculated again during sym res
    zero_out_cached_children(outModDependencies);

    // figure out path for lab modules directory
    const auto lab_mods_dir = resolve_rel_child_path_str(options->build_dir, "lab/modules");

    // for each module, let's determine its files and whether it has changed
    for(const auto mod : outModDependencies) {

        // determining module's direct files
        processor.determine_module_files(mod);

        // we must recalculate which files have changed
        mod->has_changed = std::nullopt;

        // create the module directory
        create_mod_dir(this, LabJobType::CBI, lab_mods_dir, mod);

    }

    // since caching, determine if any file has changed
    if(caching) {

        if(verbose) {
            std::cout << "[lab] " << "determining if files have changed of current module" << std::endl;
        }

        // determine if build lab has changed
        const auto has_buildLabChanged = determine_if_files_have_changed(this, module_files, buildLabObj, buildLabTimestamp);

        // if not a single module has changed, we consider it true
        bool has_any_changed = false;

        // check which modules have changed
        for (auto& dep: mod_dependencies) {
            const auto changed = has_module_changed(this, dep.module, lab_mods_dir, true);
            if (changed) {
                has_any_changed = true;
            }
        }

        if (!has_any_changed && !has_buildLabChanged) {

            // NOTE: there exists not a single module that has changed
            // also not a single file in the build.lab has changed and its object file also exists
            // which means we can safely link the previous object files again
            if(verbose) {
                std::cout << "[lab] " << "successfully reusing build lab at '" <<  buildLabObj << "'" << std::endl;
            }

            const auto state = setup_tcc_state(options->exe_path.data(), "", true, to_tcc_mode(options));
            if (state == nullptr) {
                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset;
                std::cerr << "couldn't create tcc state for jit of cached build.lab object file" << std::endl;
                return nullptr;
            }

            // add final object file (of the build.lab we cached earlier)
            if (tcc_add_file(state, buildLabObj.data()) == -1) {
                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "failed to add file '" << buildLabObj << "' in compilation of cached 'build.lab'" << std::endl;
                tcc_delete(state);
                return nullptr;
            }

            // prepare for jit
            prepare_tcc_state_for_jit(state);

            // import all compiler interfaces the modules require
            for (const auto mod: outModDependencies) {
                for (auto& interface: mod->compiler_interfaces) {
                    CompilerBinder::import_compiler_interface(interface, state);
                }
            }

            // relocate the code before calling
            if (tcc_relocate(state) == -1) {
                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "failed to relocate cached build.lab" << std::endl;
                tcc_delete(state);
                return nullptr;
            }

            return state;

        }

    }

    if(verbose) {
        std::cout << "[lab] translating each dependency module" << std::endl;
    }

    // preparing translation
    c_visitor.prepare_translate();

    // processing flattened dependencies
    for(const auto mod : outModDependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing dependency module '" << mod->format() << '\'' << std::endl;
        }

        // compile the module
        const auto module_result = process_module_tcc_bm(mod, processor, c_visitor, lab_mods_dir);
        if(module_result != 0) {
            return nullptr;
        }

    }

    if(verbose) {
        std::cout << "[lab] symbol resolving current module" << std::endl;
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = lab_processor.sym_res_module_seq(&chemical_lab_module);
    if(sym_res_status != 0) {
        return nullptr;
    }

    // type verify the module
    if(!lab_processor.type_verify_module_parallel(pool, &chemical_lab_module)) {
        return nullptr;
    }

    // the last build.lab file index (it will always be at last, unless some design change is made)
    const auto last_file_index = module_files.size() - 1;

    // allocating a name buffer, which we will use for all files
    constexpr auto nameBufferSize = 50;
    char nameBuffer[nameBufferSize];


    // what we must do is check that each build.lab gets an as_identifier + build.lab index as scope_name
    // which will mangle it differently from other build.lab files, and stop conflicts
    int i = 0;
    for (const auto file_ptr : module_files) {

        auto& file = *file_ptr;

        auto& result = file;

        // the last build.lab file is whose build method is to be called
        bool is_last = i == last_file_index;
        if (is_last) {

            auto found = find_lab_build_method(file.abs_path, file.unit.scope.body.nodes);
            if (!found) {
                return nullptr;
            }

            // TODO: we cannot verify the app build.lab method, because it
            // maybe actually a library being imported (due to a call from another build.lab)

            // expose the last file's build method, so it's callable
            found->set_specifier_fast(AccessSpecifier::Public);

        } else if (file.abs_path.ends_with(".lab") || file.abs_path.ends_with("chemical.mod")) {

            chem::string_view as_identifier = "";
            if(file.stmt == nullptr) {
#ifdef DEBUG
                CHEM_THROW_RUNTIME("stmt is nullptr");
#endif
            } else {
                as_identifier = file.stmt->getTopLevelAlias();
            }

            if (as_identifier.empty()) {

                std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset << " lab file cannot be imported without an 'as' identifier in import statement '" << file.abs_path << '\'' << std::endl;
                return nullptr;

            } else {

                auto found = find_lab_build_method(file.abs_path, file.unit.scope.body.nodes);
                if (!found) {
                    return nullptr;
                }
                if (!verify_lib_build_func_type(found, file.abs_path)) {
                    return nullptr;
                }

                // writing file index to the name buffer
                int total_written = snprintf(nameBuffer, nameBufferSize, "_%d", i);

                // debug safety check
#ifdef DEBUG
                if(total_written < 0 || total_written >= nameBufferSize) {
                    CHEM_THROW_RUNTIME("integer conversion truncated or failed.");
                }
#endif

                // building the name string, for the build.lab which will be as_identifier + '_' + file_index
                const auto name_size = as_identifier.size() + total_written;
                const auto name_str = job_allocator->allocate_released_size(name_size + 1, 1); // 1 for the null terminator
                std::memcpy(name_str, as_identifier.data(), as_identifier.size());
                std::memcpy(name_str + as_identifier.size(), nameBuffer, static_cast<size_t>(total_written));
                name_str[name_size] = '\0';

                // now we create a new module scope on the job allocator, we will set this scope as parent of this file
                // this way the mangler will think this file belongs to this module (fictionally external) and use its scope and module
                const auto new_scope = new (job_allocator->allocate_released<ModuleScope>()) ModuleScope("lab", chem::string_view(name_str, name_size), &chemical_lab_module);
                file.module = new_scope;
                file.unit.scope.set_parent(new_scope);

            }
        }

        i++;
    }

    if(verbose) {
        std::cout << "[lab] translating current module to C" << std::endl;
    }

    // translating the build.lab module
    lab_processor.translate_module(
        c_visitor, &chemical_lab_module
    );

    // end the translation
    c_visitor.end_translate();

    // compiling the c output from build.labs
    auto str = c_visitor.writer.finalized_std_view();

    // should emit c file for the build.lab
    const auto emit_c = options->emit_c && options->is_build_lab_caching_enabled;
    // the path to the c file
    const auto labOutCPath = resolve_rel_child_path_str(labModDir, "build.lab.c");

    if(emit_c) {
        writeToFile(labOutCPath, str);
    }

    if(verbose) {
        std::cout << "[lab] compiling current module to object file for caching" << std::endl;
    }

    // let's first create an object file for build.lab (for caching)
    if(options->is_build_lab_caching_enabled) {

        const auto objRes = compile_c_string(options->exe_path.data(), str.data(), buildLabObj, false, false, to_tcc_mode(options));
        if (objRes == -1) {

            // emit c if not, because error occurred
            if(!emit_c) writeToFile(labOutCPath, str);

            std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset
                      << " failed to compile 'build.lab' at '" << path << "' to object file, written to '"
                      << labOutCPath << '\'' << std::endl;

            // TODO: object file compilation failed, we must delete any existing timestamp file (so next time, we must build the build.lab from scratch)

        }

        // since object file compilation was successful, we should save a timestamp
        save_mod_timestamp(module_files, std::string_view(buildLabTimestamp), options->out_mode);
    }

    // creating a new tcc state
    const auto state = setup_tcc_state(options->exe_path.data(), "", true, to_tcc_mode(options));
    if(state == nullptr) {
        // emit c if not, because error occurred
        if(!emit_c) writeToFile(labOutCPath, str);
        std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset << "couldn't create tcc state for 'build.lab' file at '" << path << "', written to '" << labOutCPath << '\'' << std::endl;
        return nullptr;
    }

    // compiling the program
    if(tcc_compile_string(state, str.data()) == -1) {
        // emit c if not, because error occurred
        if(!emit_c) writeToFile(labOutCPath, str);
        std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " couldn't compile 'build.lab' at '" << path << "', written to '" << labOutCPath << '\'' << std::endl;
        tcc_delete(state);
        return nullptr;
    }

    // prepare for jit
    prepare_tcc_state_for_jit(state);

    // import all compiler interfaces the lab files import
    // import all compiler interfaces the modules require
    for(const auto mod : outModDependencies) {
        for(auto& interface : mod->compiler_interfaces) {
            CompilerBinder::import_compiler_interface(interface, state);
        }
    }

    // add all the link link libraries required
    for(auto& linkLib : job->link_libs) {
        if(tcc_add_library(state, linkLib.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't link library '" << linkLib << "' for build.lab at '" << path << '\'' << std::endl;
        }
    }

    // relocate the code before calling
    tcc_relocate(state);

    // clear the output ptr
    c_visitor.writer.clear();

    // clear the allocators
    mod_allocator->clear();
    file_allocator->clear();

    // return the state
    return state;

}

LabModule* LabBuildCompiler::built_mod_file(LabBuildContext& context, const std::string_view& path, LabJob* job) {

    // create a directory for lab processing and dependent modules
    // we'll call it 'lab' inside the build directory
    const auto lab_dir = resolve_rel_child_path_str(options->build_dir, "lab");
    const auto lab_mods_dir = resolve_rel_child_path_str(lab_dir, "modules");

    // create lab dir and modules dir inside lab dir (lab, lab/modules)
    create_dir(lab_dir);
    create_dir(lab_mods_dir);

    // call the function
    const auto result = build_module_from_mod_file(
            context, path, job
    );

    return result;

}

int LabBuildCompiler::do_allocating(void* data, int(*do_jobs)(LabBuildCompiler*, void*)) {

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // we switch these back, just in case, we had allocators already
    auto prev_job_allocator = job_allocator;
    auto prev_mod_allocator = mod_allocator;
    auto prev_file_allocator = file_allocator;

    // the allocators that will be used for all jobs
    set_allocators(&_job_allocator, &_mod_allocator, &_file_allocator);

    // do the jobs
    const auto result = do_jobs(this, data);

    // set allocators back to previous
    set_allocators(prev_job_allocator, prev_mod_allocator, prev_file_allocator);

    return result;

}

int LabBuildCompiler::do_allocating(const std::function<int()>& fn) {

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // we switch these back, just in case, we had allocators already
    auto prev_job_allocator = job_allocator;
    auto prev_mod_allocator = mod_allocator;
    auto prev_file_allocator = file_allocator;

    // the allocators that will be used for all jobs
    set_allocators(&_job_allocator, &_mod_allocator, &_file_allocator);

    // do the jobs
    const auto result = fn();

    // set allocators back to previous
    set_allocators(prev_job_allocator, prev_mod_allocator, prev_file_allocator);

    return result;
}

int LabBuildCompiler::do_job_allocating(LabJob* job) {

    return do_allocating((void*) job, [](LabBuildCompiler* compiler, void* data) {

        const auto job = (LabJob*) data;
        return compiler->do_job(job);

    });

}

TCCState* LabBuildCompiler::built_lab_file(
        LabBuildContext& context,
        const std::string_view& path,
        bool mod_file_source
) {

    // this is host target data
    // since we are generating code for the target system
    // compiler specific target data is fine
    auto targetData = create_target_data();

    // a global interpret scope required to evaluate compile time things
    // empty target triple, because we are targeting the current system
    GlobalInterpretScope global(options->out_mode, targetData, nullptr, this, *job_allocator, type_builder, loc_man);

    // instantiations container holds the types
    InstantiationsContainer instContainer;

    // creating symbol resolver for build.lab files only
    SymbolResolver lab_resolver(binder, global, path_handler, controller, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // the processor that does everything for build.lab files only
    ASTProcessor lab_processor(
            path_handler,
            options,
            mod_storage,
            controller,
            loc_man,
            &lab_resolver,
            binder,
            type_builder,
            *job_allocator,
            *mod_allocator,
            *file_allocator
    );

    // creates or rebinds the global container
    // empty target triple (current system)
    create_or_rebind_container(this, global, lab_resolver, targetData);

    // compiler interfaces the lab files imports
    ToCAstVisitor c_visitor(binder, global, mangler, *file_allocator, loc_man, options->debug_info, options->minify_c);
    ToCBackendContext c_context(&c_visitor);

    // set the backend context
    global.backend_context = &c_context;

    // create a directory for lab processing and dependent modules
    // we'll call it 'lab' inside the build directory
    const auto lab_dir = resolve_rel_child_path_str(options->build_dir, "lab");
    const auto lab_mods_dir = resolve_rel_child_path_str(lab_dir, "modules");

    // create lab dir and modules dir inside lab dir (lab, lab/modules)
    create_dir(lab_dir);
    create_dir(lab_mods_dir);

    // lets create the job for jit of build.lab/chemical.mod file
    LabJob build_job(LabJobType::JITExecutable, chem::string("build"), chem::string(""), chem::string(options->build_dir), options->def_out_mode);
    LabBuildContext::initialize_job(&build_job, options);;

    // get build lab file into a tcc state
    const auto state = built_lab_file(
            context, path, lab_processor, c_visitor, mod_file_source, &build_job
    );

    return state;

}

int LabBuildCompiler::build_lab_file_no_alloc(LabBuildContext& context, const std::string_view& path) {

    // mkdir the build directory
    create_dir(options->build_dir);

    // get build lab file into a tcc state
    const auto state = built_lab_file(context, path, false);
    if(!state) {
        return 1;
    }

    // automatic destroy
    TCCDeletor auto_del(state);

    // allocators
    auto& _job_allocator = *job_allocator;
    auto& _mod_allocator = *mod_allocator;
    auto& _file_allocator = *file_allocator;

    // clear everything from allocators before proceeding
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    // get the build method
    auto build = (void(*)(LabBuildContext*)) tcc_get_symbol(state, "chemical_lab_build");
    if(!build) {
        std::cerr << rang::fg::red << "[lab] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
        return 1;
    }

    // clear the module storage
    // these modules were created to facilitate the build.lab generation
    // if not cleared, these modules will interfere with modules created for executable
    context.storage.clear();

    // call the root build.lab build's function
    build(&context);

    int job_result = 0;

    // generating outputs (executables)
    for(auto& exe : context.executables) {

        job_result = do_job(exe.get());
        if(job_result != 0) {
            std::cerr << rang::fg::red << "[lab] " << "error performing job '" << exe->name.data() << "', returned status code " << job_result << rang::fg::reset << std::endl;
            break;
        }

        // clearing all allocations done in all the allocators
        _job_allocator.clear();
        _mod_allocator.clear();
        _file_allocator.clear();

    }

    // running the on_finished lambda
    if(job_result == 0 && context.on_finished) {
        context.on_finished(context.on_finished_data);
    }

    return job_result;

}

int LabBuildCompiler::build_module_build_file_no_alloc(
        LabBuildContext& context,
        const std::string_view& path,
        LabJob* final_job,
        bool mod_file_source,
        bool promote_to_app
) {

    // mkdir the build directory
    create_dir(options->build_dir);

    // get chemical.mod file into a tcc state
    const auto state = built_lab_file(context, path, mod_file_source);
    if(!state) {
        return 1;
    }

    // automatic destroy
    TCCDeletor auto_del(state);

    // allocators
    auto& _job_allocator = *job_allocator;
    auto& _mod_allocator = *mod_allocator;
    auto& _file_allocator = *file_allocator;

    // clear everything from allocators before proceeding
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    // get the build method
    auto build = (LabModule*(*)(LabBuildContext*, LabJob*)) tcc_get_symbol(state, "chemical_lab_build");
    if(!build) {
        std::cerr << rang::fg::red << "[lab] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
        return 1;
    }

    // clear the module storage
    // these modules were created to facilitate the build.lab generation
    // if not cleared, these modules will interfere with modules created for executable
    context.storage.clear();

    // call the root chemical.mod build's function
    const auto main_module = build(&context, final_job);
    if(main_module == nullptr) {
        return 1;
    }

    // if running via 'chemical run', promote library module to application
    // so the main function becomes the entry point (no_mangle)
    if(promote_to_app && main_module && main_module->package_kind == PackageKind::Library) {
        main_module->package_kind = PackageKind::Application;
    }

    // just put main the module as this job's dependency
    final_job->add_dependency(main_module);

    // lets compile any cbi jobs user may have specified
    for(auto& job : context.executables) {
        if(job->type == LabJobType::CBI) {
            const auto job_result = do_job(job.get());
            if(job_result != 0) {
                return job_result;
            }
        }
    }

    const auto job_result = do_job(final_job);
    if(job_result != 0) {
        std::cerr << rang::fg::red << "[lab] " << "error emitting executable, returned status code 1" << rang::fg::reset << std::endl;
    }

    // clearing all allocations done in all the allocators
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    return job_result;

}

std::string LabBuildCompiler::get_commands_cache_dir() {
    auto home = getUserHomeDirectory();
    if (home.empty()) return "";
    auto chemical_dir = resolve_rel_child_path_str(home, ".chemical");
    return resolve_rel_child_path_str(chemical_dir, "commands");
}

int LabBuildCompiler::run_invocation(
    const std::string& compiler_exe_path,
    const std::string& target,
    const std::vector<std::string_view>& args,
    OutputMode mode,
    CmdOptions* cmd_options
) {
    bool is64Bit = true; // assume 64 bit for now, should be passed from options if possible
#ifdef _WIN32
    #ifndef _WIN64
        is64Bit = false;
    #endif
#endif
    
    // For local projects, build in the current directory's build folder
    // For remote/transformers, we'll override this once we have the module name
    std::string build_dir = "build";
    LabBuildCompilerOptions compiler_opts(compiler_exe_path, "", std::move(build_dir), is64Bit);
    compiler_opts.out_mode = mode;
    compiler_opts.def_out_mode = mode;
    
    CompilerBinder binder(compiler_exe_path);
    LocationManager loc_man;
    LabBuildCompiler compiler(loc_man, binder, &compiler_opts);
    compiler.set_cmd_options(cmd_options);
    
    LabBuildContext context(compiler, compiler.path_handler, compiler.mod_storage, compiler.binder);
    
    // Detection logic
    // 1. Is it a transformer? (Multiple args, first is likely a module, second is a file)
    if (args.size() >= 1) {
        auto& first_arg = target;
        std::string second_arg(args[0]);
        
        bool first_is_file = first_arg.ends_with(".lab") || first_arg.ends_with(".mod");
        bool second_is_file = second_arg.ends_with(".lab") || second_arg.ends_with(".mod");
        
        if (!first_is_file && second_is_file) {
            // Probably a transformer
            std::vector<std::string_view> transformer_args;
            for(size_t i = 1; i < args.size(); ++i) transformer_args.push_back(args[i]);

            return compiler.do_allocating([&]() -> int {
                return compiler.run_transformer(first_arg, second_arg, transformer_args, context);
            });
        }
    }
    
    // 2. Is it a local project?
    if (target.ends_with(".lab") || target.ends_with(".mod")) {
        return compiler.do_allocating([&]() -> int {
            compiler_opts.build_dir = resolve_sibling(target, "build");
#ifdef _WIN32
            auto exe_path = resolve_rel_child_path_str(compiler_opts.build_dir, "run.exe");
#else
            auto exe_path = resolve_rel_child_path_str(compiler_opts.build_dir, "run");
#endif
            return compiler.run_local_project(target, chem::string(exe_path), args, context);
        });

    }
    
    // 3. It's a remote module run
    return compiler.do_allocating([&]() -> int {
        return compiler.run_remote_module(target, args, context);
    });
}

static int launch_process_and_wait(const char* exe_str, const std::vector<std::string_view>& args) {
    // Run the executable
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(exe_str));
    for (const auto& arg : args) {
        argv.push_back(const_cast<char*>(arg.data()));
    }
    argv.push_back(nullptr);
    return launch_process_and_wait(exe_str, argv.data(), argv.size() - 1);
}

int LabBuildCompiler::run_local_project(
        const std::string& target,
        chem::string outputPath,
        const std::vector<std::string_view>& args,
        LabBuildContext& context
) {

    auto& opts = *options;
    LabJob final_job(LabJobType::Executable, chem::string("main"), std::move(outputPath), chem::string(opts.build_dir), opts.out_mode);
    LabBuildContext::initialize_job(&final_job, &opts);
    const auto result = build_module_build_file_no_alloc(context, target, &final_job, !target.ends_with(".lab"), true);
    if (result == 0) {
        // Run the executable
        return launch_process_and_wait(final_job.abs_path.data(), args);
    }
    return result;
}

static int parse_version_part(std::string_view& v) {
    int res = 0;
    while (!v.empty() && isdigit(v[0])) {
        res = res * 10 + (v[0] - '0');
        v.remove_prefix(1);
    }
    return res;
}

int LabBuildCompiler::compare_remote_versions(const chem::string_view& v1_raw, const chem::string_view& v2_raw) {
    if (v1_raw == v2_raw) return 0;
    if (v1_raw.empty()) return -1;
    if (v2_raw.empty()) return 1;

    auto v1 = v1_raw.view();
    auto v2 = v2_raw.view();

    // Skip 'v' prefix if present
    if (!v1.empty() && v1[0] == 'v') v1.remove_prefix(1);
    if (!v2.empty() && v2[0] == 'v') v2.remove_prefix(1);

    while (!v1.empty() || !v2.empty()) {
        if (!v1.empty() && !isdigit(v1[0]) && v1[0] != '.') return -2;
        if (!v2.empty() && !isdigit(v2[0]) && v2[0] != '.') return -2;

        int n1 = parse_version_part(v1);
        int n2 = parse_version_part(v2);

        if (n1 < n2) return -1;
        if (n1 > n2) return 1;

        if (!v1.empty() && v1[0] == '.') v1.remove_prefix(1);
        if (!v2.empty() && v2[0] == '.') v2.remove_prefix(1);

        if (v1.empty() && v2.empty()) break;
    }

    return 0;
}

bool LabBuildCompiler::parse_remote_import_from(RemoteImport& import, ASTAllocator& allocator) {
    if (!import.mod_scope.empty() && !import.mod_name.empty()) {
        if (import.origin.empty()) {
             import.origin = chem::string_view("github.com");
        }
        return true;
    }

    auto from_path_view = import.from.view();
    if (from_path_view.empty()) return false;

    auto last_slash = from_path_view.find_last_of('/');
    
    chem::string mod_scope, mod_name, origin;
    bool has_remote_origin = false;

    if (last_slash != std::string_view::npos) {
        mod_name.append(from_path_view.substr(last_slash + 1));
        auto parent = from_path_view.substr(0, last_slash);
        auto second_slash = parent.find_last_of('/');
        if (second_slash != std::string_view::npos) {
             has_remote_origin = true;
             mod_scope.append(parent.substr(second_slash + 1));
             origin.append(parent.substr(0, second_slash));
        } else {
             mod_scope.append(parent);
             origin.append("github.com");
        }
    } else {
        mod_name.append(from_path_view);
        origin.append("github.com");
    }

    if (import.mod_scope.empty())
        import.mod_scope = { allocator.allocate_str(mod_scope.data(), mod_scope.size()) };
    if (import.mod_name.empty())
        import.mod_name = { allocator.allocate_str(mod_name.data(), mod_name.size()) };
    if (import.origin.empty()) {
        import.origin = { allocator.allocate_str(origin.data(), origin.size()) };
    }

    return !import.mod_name.empty();
}

struct RemoteRepoInfo {
    std::string storage_path;
    std::string target_dir;
};

static RemoteRepoInfo get_remote_repo_info(const std::string& remote_mods_dir, const RemoteImport& import) {
    RemoteRepoInfo info;
    auto& path = info.storage_path;
    path += import.origin.view();
    path += '/';
    path += import.mod_scope.view();
    path += '/';
    path += import.mod_name.view();
    info.target_dir = resolve_rel_child_path_str(remote_mods_dir, info.storage_path);
    return info;
}

int LabBuildCompiler::run_remote_module(const std::string& target, const std::vector<std::string_view>& args, LabBuildContext& context) {

    auto cache_dir = get_commands_cache_dir();
    if (cache_dir.empty()) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't determine home directory for caching" << std::endl;
        return 1;
    }

    // the target dir is where we should store executable
    // inside target dir, create a build dir, which we should use for the build
    RemoteImport import { .from = chem::string_view(target) };
    if(!parse_remote_import_from(import, global_allocator)) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't parse the remote module string" << std::endl;
        return 1;
    }
    auto info = get_remote_repo_info(cache_dir, import);

    // determine output path of the executable
    auto outputPath = chem::string();
    outputPath.append(info.target_dir);
    outputPath.append("/");
#ifdef _WIN32
    outputPath.append("run.exe");
#else
    outputPath.append("run");
#endif

    // lets check if executable already exists and invoke it
    // without downloading it
    if(fs::exists(outputPath.to_view())) {
        // executable already exists
        return launch_process_and_wait(outputPath.data(), args);
    }

    // we will delete the build dir, this contains everything (remote modules, intermediate objects)
    auto build_dir = resolve_rel_child_path_str(info.target_dir, "build");

    // Ensure cache dir exists
    fs::create_directories(build_dir);

    // creating a executable job
    LabJob final_job(LabJobType::Executable, chem::string("main"), options->out_mode);
    // this is where 'remote' directory will be created by process_remote_imports
    // we use build_dir, since we want to delete the downloaded sources at the end
    final_job.build_dir = chem::string(build_dir);
    // setting the absolute path to the executable
    final_job.abs_path.append(outputPath.to_view());

    // add the user's requested command module as a remote import to the job
    auto sym_info = (DependencySymbolInfo*) global_allocator.allocate_released_size(sizeof(DependencySymbolInfo), alignof(DependencySymbolInfo));
    *sym_info = { .location = 0 };
    import.requesters.push_back({ nullptr, sym_info });
    if (!add_remote_import(&final_job, import)) return 1;


    // this will process remote imports recursively, and add to the job
    if (process_remote_imports(context, &final_job) != 0) {
        return 1;
    }

    // disable caching, because it causes generation of .dat and partial 2c files
    // we don't want these files being generated in the cache build directory
    options->is_caching_enabled = false;
    // disable build lab caching, we don't want to generate its .dat and partial 2c files
    options->is_build_lab_caching_enabled = false;

    // if remote imports succeeded, it must have lead to a single module
    // the first one
    if (final_job.dependencies.empty()) return 1;
    auto mod = final_job.dependencies[0].module;

    // clearing the remote imports from the final job
    // we've downloaded them, we still use do_job
    // do_job will again process remote imports, when we can just skip that
    final_job.remote_imports.clear();

    // since we are running it
    // we should ensure its main function automatically gets no_mangle
    mod->package_kind = PackageKind::Application;

    // lets compile any cbi jobs user may have specified
    for(auto& job : context.executables) {
        if(job->type == LabJobType::CBI) {
            const auto job_result = do_job(job.get());
            if(job_result != 0) {
                return job_result;
            }
        }
    }

    // do the actual job
    const auto job_result = do_job(&final_job);
    if(job_result == 0) {
        // removes downloaded sources + intermediate objects
        fs::remove_all(build_dir);
    } else {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "emitting executable, returned status code 1" << std::endl;
    }

    // get allocators
    auto& _job_allocator = *job_allocator;
    auto& _mod_allocator = *mod_allocator;
    auto& _file_allocator = *file_allocator;

    // clearing all allocations done in all the allocators
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    // end if compilation failed
    if(job_result != 0) {
        return job_result;
    }

    // Run the executable
    return launch_process_and_wait(final_job.abs_path.data(), args);

}

int LabBuildCompiler::local_or_remote_project_to_module(
        LabJob* job,
        const std::string& target,
        const std::string& cache_dir,
        LabBuildContext& context
) {
    const auto is_mod_transformer = target.ends_with(".mod");
    if (!is_mod_transformer && !target.ends_with(".lab")) {

        // the target dir is where we should store executable
        // inside target dir, create a build dir, which we should use for the build
        RemoteImport import { .from = chem::string_view(target) };
        if(!parse_remote_import_from(import, global_allocator)) {
            std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't parse the remote module string" << std::endl;
            return 1;
        }
        auto info = get_remote_repo_info(cache_dir, import);

        // we will delete the build dir, this contains everything (remote modules, intermediate objects)
        auto build_dir = resolve_rel_child_path_str(info.target_dir, "build");

        // Ensure cache dir exists
        fs::create_directories(build_dir);

        // this is where 'remote' directory will be created by process_remote_imports
        // we use build_dir, since we want to delete the downloaded sources at the end
        job->build_dir = chem::string(build_dir);

        // add the user's requested command module as a remote import to the job
        auto sym_info = (DependencySymbolInfo*) global_allocator.allocate_released_size(sizeof(DependencySymbolInfo), alignof(DependencySymbolInfo));
        *sym_info = { .location = 0 };
        import.requesters.push_back({ nullptr, sym_info });
        if (!add_remote_import(job, import)) return 1;

        // this will process remote imports recursively, and add to the job
        if (process_remote_imports(context, job) != 0) {
            return 1;
        }

        // disable caching, because it causes generation of .dat and partial 2c files
        // we don't want these files being generated in the cache build directory
        options->is_caching_enabled = false;
        // disable build lab caching, we don't want to generate its .dat and partial 2c files
        options->is_build_lab_caching_enabled = false;

        // clearing the remote imports from the final job
        // we've downloaded them, we still use do_job
        // do_job will again process remote imports, when we can just skip that
        job->remote_imports.clear();

    } else {

        // get chemical.mod/build.lab file into a tcc state
        const auto state = built_lab_file(context, target, is_mod_transformer);
        if(!state) {
            return 1;
        }

        // automatic destroy
        TCCDeletor auto_del(state);

        // get the build method
        auto build = (LabModule*(*)(LabBuildContext*, LabJob*)) tcc_get_symbol(state, "chemical_lab_build");
        if(!build) {
            std::cerr << rang::fg::red << "[lab] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
            return 1;
        }

        // clear the module storage
        // these modules were created to facilitate the build.lab generation
        // if not cleared, these modules will interfere with modules created for executable
        context.storage.clear();

        // call the root chemical.mod build's function
        const auto main_module = build(&context, job);
        if(main_module == nullptr) {
            return 1;
        }

        // just put main the module as this job's dependency
        job->add_dependency(main_module);

    }

    return 0;

}

std::string get_transformers_cache_dir() {
    auto home = getUserHomeDirectory();
    if (home.empty()) return "";
    auto chemical_dir = resolve_rel_child_path_str(home, ".chemical");
    return resolve_rel_child_path_str(chemical_dir, "transformers");
}


int LabBuildCompiler::run_transformer(const std::string& transformer, const std::string& target, const std::vector<std::string_view>& args, LabBuildContext& context) {

    // the cbi job for the transformer module
    // created so we can store remote imports here
    LabJobCBI transformer_job(chem::string("main"), options->out_mode);

    // It's a remote transformer, download it
    auto cache_dir = get_transformers_cache_dir();
    if (cache_dir.empty()) return 1;

    // create and set build directory
    auto build_dir_path = resolve_rel_child_path_str(cache_dir, transformer);
    fs::create_directories(build_dir_path);
    options->build_dir = build_dir_path;
    transformer_job.build_dir = chem::string(build_dir_path);

    // native transformer names
    bool is_native = false;
    if(transformer == "refgen") {
        is_native = true;
    }

    if(is_native) {
        auto native_path = path_handler.resolve_native_lib(chem::string_view(transformer));
        if(native_path.empty()) {
            std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't find native transformer '" << transformer << "'" << std::endl;
            return 1;
        }

        // search for a build file (chemical.mod or build.lab)
        bool is_mod_file_source = true;
        auto build_file_path = native_path + "/chemical.mod";
        if(!fs::exists(build_file_path)) {
            build_file_path = native_path + "/build.lab";
            if(fs::exists(build_file_path)) {
                is_mod_file_source = false;
            } else {
                std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't find a build file 'chemical.mod' or 'build.lab' for '" << transformer << "'" << std::endl;
                return 1;
            }
        }

        const auto status = local_or_remote_project_to_module(&transformer_job, build_file_path, cache_dir, context);
        if(status != 0) return status;

    } else {

        // compile transformer job to a module
        const auto status = local_or_remote_project_to_module(&transformer_job, transformer, cache_dir, context);
        if(status != 0) return status;

    }

    // first lets execute any cbi jobs, present before, except the final job (done explicitly)
    for(auto& job : context.executables) {
        if(job->type == LabJobType::CBI && job.get() != &transformer_job) {
            const auto job_result = do_job(job.get());
            if(job_result != 0) {
                return job_result;
            }
        }
    }

    // allocators
    auto& _job_allocator = *job_allocator;
    auto& _mod_allocator = *mod_allocator;
    auto& _file_allocator = *file_allocator;

    // clearing all allocations done in all the allocators
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    // do the actual job
    const auto result = do_job(&transformer_job);
    if(result != 0) {
        return result;
    }

    // clear everything, since now cbi has been built
    mod_storage.clear();
    buildLabDependenciesCache.clear();
    // clearing all allocations done in all the allocators
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();
    // clear the exists cache
    context.existance_cache.clear();

    // check other transformer contains at least a single module
    if(transformer_job.dependencies.empty()) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't find the transformer module" << std::endl;
        return 1;
    }

    // creating a job for the target
    LabJob other_job(LabJobType::ProcessingOnly, chem::string("target"), options->out_mode);
    other_job.build_dir = chem::string(build_dir_path);

    // disable caching, we must always reparse for a transformation job
    options->is_caching_enabled = false;
    options->is_build_lab_caching_enabled = false;

    // compile the other module job to a module
    auto target_to_mod_status = local_or_remote_project_to_module(&other_job, target, cache_dir, context);
    if(target_to_mod_status != 0) {
        return target_to_mod_status;
    }

    // check other job contains at least a single module
    if(other_job.dependencies.empty()) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't find the target module" << std::endl;
        return 1;
    }

    // an interpretation scope for interpreting compile time function calls
    GlobalInterpretScope global(other_job.mode, other_job.target_data, nullptr, this, *job_allocator, type_builder, loc_man);

    // we hold the instantiated types inside this container
    InstantiationsContainer instContainer;

    // a new symbol resolver for every executable
    SymbolResolver resolver(binder, global, path_handler, controller, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // creating a ast processor is required
    ASTProcessor processor(path_handler, options, mod_storage, controller, loc_man, &resolver, binder, type_builder, *job_allocator, *mod_allocator, *file_allocator);

    // create or rebind the global container (comptime functions like intrinsics namespace)
    create_or_rebind_container(this, global, resolver, other_job.target_data);

    // flatten the modules of other job, which is going to be processed by transformer
    auto outMods = flatten_dedupe_sorted(other_job.dependencies);

    // figuring out direct files in each module
    // for each module, let's determine its files and whether it has changed
    for(const auto mod : outMods) {

        // determining module's direct files
        processor.determine_module_files(mod);

    }

    // the transformer context is passed
    TransformerContext transformer_context(&other_job, this, &processor, std::move(outMods));

    // lets see if the transformer init function is available
    const auto transformer_main_fn = binder.findHook("transformer_main", CBIFunctionType::TransformerMain);
    if(transformer_main_fn == nullptr) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't find function transformer_main" << std::endl;
        return 1;
    }

    // Invoke transformer_init
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(transformer.c_str()));
    for (const auto& arg : args) argv.push_back(const_cast<char*>(arg.data()));
    argv.push_back(nullptr);

    // transformer main
    const auto transformer_main = (int(*)(TransformerContext*, int, char**)) transformer_main_fn;

    // transformer context would automatically allow us to compile the module however we want
    return transformer_main(&transformer_context, (int)argv.size() - 1, argv.data());

}


LabBuildCompiler::~LabBuildCompiler() {
    GlobalInterpretScope::dispose_container(container);
}

bool LabBuildCompiler::add_remote_import(LabJob* job, RemoteImport& import, ConflictResolutionStrategy strategy) {
    if (!parse_remote_import_from(import, global_allocator)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(job_mutex);

    std::string key;
    key.reserve(import.origin.size() + import.mod_scope.size() + import.mod_name.size() + 2);
    key.append(import.origin.view());
    key.append("/");
    key.append(import.mod_scope.view());
    key.append("/");
    key.append(import.mod_name.view());

    auto it_idx = job->remote_import_index.find(key);
    if (it_idx != job->remote_import_index.end()) {
        auto& existing = job->remote_imports[it_idx->second];
        bool keep_existing = true;
        int res = resolve_remote_import_conflict(strategy == ConflictResolutionStrategy::Default ? job->conflict_strategy : strategy, existing, import, keep_existing);
        if (res != 0) return false;
        
        // merge requesters
        std::vector<RemoteImportRequester> merged_requesters = std::move(existing.requesters);
        for(auto& req : import.requesters) {
            merged_requesters.push_back(req);
        }

        if (!keep_existing) {
            existing = std::move(import);
        }
        existing.requesters = std::move(merged_requesters);
        return true;
    }

    job->remote_import_index[key] = job->remote_imports.size();
    job->remote_imports.push_back(std::move(import));
    return true;
}

int LabBuildCompiler::resolve_remote_import_conflict(ConflictResolutionStrategy strategy, RemoteImport& existing, RemoteImport& current, bool& keep_existing) {
    if (strategy == ConflictResolutionStrategy::OverridePrevious) {
        keep_existing = false;
        return 0;
    }
    if (strategy == ConflictResolutionStrategy::KeepPrevious) {
        keep_existing = true;
        return 0;
    }

    // 1 - if the versions / commit hash / branch are same, this means its just a duplicate
    if (existing.version == current.version && 
        existing.branch == current.branch && 
        existing.commit == current.commit &&
        existing.subdir == current.subdir) {
        keep_existing = true;
        return 0;
    }

    if (strategy == ConflictResolutionStrategy::RaiseError) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "version conflict detected for remote import '" << existing.from << "'\n";
        std::cerr << "  first requested version: " << (existing.version.empty() ? (existing.commit.empty() ? existing.branch : existing.commit) : existing.version) << "\n";
        // for now just show the first requester's location
        if (!existing.requesters.empty())
            std::cerr << " at " << loc_man.formatLocation(existing.requesters[0].symbol_info->location) << '\n';
        std::cerr << "  second requested version: " << (current.version.empty() ? (current.commit.empty() ? current.branch : current.commit) : current.version) << "\n";
        if (!current.requesters.empty())
            std::cerr << " at " << loc_man.formatLocation(current.requesters[0].symbol_info->location) << '\n';
        return 1;
    }

    int cmp = compare_remote_versions(existing.version, current.version);
    if (cmp == -2) {
        // Fallback to RaiseError if versions are incomparable
        return resolve_remote_import_conflict(ConflictResolutionStrategy::RaiseError, existing, current, keep_existing);
    }

    if (strategy == ConflictResolutionStrategy::PreferNewerVersion) {
        keep_existing = (cmp >= 0);
    } else if (strategy == ConflictResolutionStrategy::PreferOlderVersion) {
        keep_existing = (cmp <= 0);
    } else {
        // Default (was hopefully resolved to a specific strategy above, but if not, prefer newer)
        keep_existing = (cmp >= 0);
    }

    return 0;
}

struct RemoteImportProgress {
    std::mutex mutex;
    std::atomic<int> total;
    std::atomic<int> completed{0};
    int last_width{0};
    bool is_first_time_printing = true;
    std::vector<std::string> errors;

    RemoteImportProgress(int total) : total(total) {}

    void add_to_total(int n) {
        total += n;
    }

    void add_error(std::string msg) {
        std::lock_guard<std::mutex> lock(mutex);
        errors.push_back(std::move(msg));
    }

    void update() {
        int done = ++completed;
        int current_total = total.load();
        if (current_total == 0) return;

        std::lock_guard<std::mutex> lock(mutex);

        float progress = (float)done / current_total;
        int bar_width = 30;
        int pos = (int)(bar_width * progress);

        std::string output;
        output.reserve(128);

        if (!is_first_time_printing)
            output += '\r';
        else
            is_first_time_printing = false;

        output += "[lab] downloading [";

        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) output += '=';
            else if (i == pos) output += '>';
            else output += ' ';
        }

        output += "] ";
        output += std::to_string((int)(progress * 100));
        output += "% (";
        output += std::to_string(done);
        output += "/";
        output += std::to_string(current_total);
        output += ")";

        if (output.size() < last_width)
            output.append(last_width - output.size(), ' ');

        last_width = output.size();

        std::cout << output << std::flush;
    }

    void finish() {
        int current_total = total.load();
        if (completed >= current_total && errors.empty() && current_total > 0) {
            // Keep the 100% status on screen and move to next line
            std::cout << std::endl;
        } else {
            // Clear the line if there were errors or if it was interrupted
            std::cout << "\r";
            for (int i = 0; i < last_width; ++i) std::cout << " ";
            std::cout << "\r" << std::flush;
        }
        if (!errors.empty()) {
            for (const auto& err : errors) {
                std::cerr << err;
            }
        }
    }
};

static int run_git_and_capture(const std::string& cmd, std::string& output) {
#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif
    if (!pipe) return -1;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        output += buffer;
    }
#ifdef _WIN32
    return _pclose(pipe);
#else
    return pclose(pipe);
#endif
}

static int download_remote_import(
    LabBuildContext& context,
    LabBuildCompiler* compiler,
    LabJob* job,
    RemoteImport* import,
    RemoteImportProgress& progress
) {
    const auto job_build_dir = job->build_dir.to_std_string();
    auto remote_mods_dir = resolve_rel_child_path_str(job_build_dir, "remote");

    auto info = get_remote_repo_info(remote_mods_dir, *import);
    auto target_dir = info.target_dir;

    if(!fs::exists(target_dir)) {
        fs::create_directories(fs::path(target_dir).parent_path());

        auto url = info.storage_path;
        if (url.find("://") == std::string::npos && url.find("git@") != 0) {
            url = "https://" + url;
        }

        std::string git_base = "git -c advice.detachedHead=false ";
        std::vector<std::string> arg_list;
        
        if(!import->commit.empty()) {
             fs::create_directories(target_dir);
             auto commit_str = import->commit.str();
             arg_list.push_back("-C \"" + target_dir + "\" init --quiet");
             arg_list.push_back("-C \"" + target_dir + "\" remote add origin \"" + url + "\"");
             arg_list.push_back("-C \"" + target_dir + "\" fetch --quiet --depth 1 origin \"" + commit_str + "\"");
             arg_list.push_back("-C \"" + target_dir + "\" checkout --quiet FETCH_HEAD");
        } else {
            auto& version_or_branch = import->version.empty() ? import->branch : import->version;
            if(version_or_branch.empty()) {
                arg_list.push_back("clone --quiet --depth 1 \"" + url + "\" \"" + target_dir + "\"");
            } else {
                arg_list.push_back("clone --quiet --depth 1 --branch \"" + version_or_branch.str() + "\" \"" + url + "\" \"" + target_dir + "\"");
            }
        }

        for (const auto& args : arg_list) {
            std::string cmd = git_base + args + " 2>&1";
            std::string output;
            int result = run_git_and_capture(cmd, output);
            if (result != 0) {
                std::stringstream ss;
                ss << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "failed to download remote import";
                ss << " from '" << import->from << "'\n";
                ss << "[git] " << output << "\n";
                progress.add_error(ss.str());
                return result;
            }
        }
    }

    ModuleDependencyRecord record{ "" };
    if (import->subdir.empty()) {
        record.module_dir_path = target_dir;
    } else {
        record.module_dir_path = resolve_rel_child_path_str(target_dir, import->subdir.view());
    }

    auto mod = compiler->create_module_for_dependency(context, record, job);

    if (mod) {
        if(mod->scope_name != import->mod_scope) {
            mod->scope_name.clear();
            mod->scope_name.append(import->mod_scope);
        }
        if(mod->name != import->mod_name) {
            mod->name.clear();
            mod->name.append(import->mod_name);
        }

        std::lock_guard<std::mutex> lock(compiler->job_mutex);
        for (auto& req_info : import->requesters) {
            if(req_info.requester) {
                req_info.requester->add_dependency(mod, req_info.symbol_info);
            } else {
                job->add_dependency(mod, req_info.symbol_info);
            }
        }
        return 0;
    } else {
        std::stringstream ss;
        ss << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "failed to initialize module after download from '" << import->from << "'\n";
        progress.add_error(ss.str());
        return 1;
    }
}

int LabBuildCompiler::process_remote_imports(LabBuildContext& context, LabJob* job) {
    if(job->remote_imports.empty()) {
        return 0;
    }

    RemoteImportProgress progress(0);
    size_t processed_count = 0;
    int final_status = 0;

    while (true) {
        std::vector<RemoteImport*> wave_ptrs;
        {
            std::lock_guard<std::mutex> lock(job_mutex);
            if (processed_count >= job->remote_imports.size()) {
                break;
            }
            for (size_t i = processed_count; i < job->remote_imports.size(); ++i) {
                wave_ptrs.push_back(&job->remote_imports[i]);
            }
            processed_count = job->remote_imports.size();
        }

        if (wave_ptrs.empty()) break;

        progress.add_to_total((int)wave_ptrs.size());
        std::cout << "[lab] processing " << wave_ptrs.size() << " remote imports (wave)" << std::endl;

        std::vector<std::future<int>> futures;
        futures.reserve(wave_ptrs.size());

        for(auto import : wave_ptrs) {
            futures.emplace_back(pool.push([this, &context, job, import, &progress](int id) {
                auto result = download_remote_import(context, this, job, import, progress);
                progress.update();
                return result;
            }));
        }

        for(auto& f : futures) {
            auto status = f.get();
            if(status != 0) {
                final_status = status;
            }
        }

        if (final_status != 0) break;
    }
    
    progress.finish();
    return final_status;
}
