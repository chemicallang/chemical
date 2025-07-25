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
#include "ast/structures/If.h"
#include "utils/Benchmark.h"
#include "ast/structures/ModuleScope.h"
#include "Utils.h"
#include "core/source/LocationManager.h"
#include <fstream>
#include <span>
#include "compiler/lab/mod_conv/ModToLabConverter.h"
#include "parser/utils/ParseModDecl.h"
#include "compiler/lab/timestamp/Timestamp.h"
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
#include "ast/base/TypeBuilder.h"

#ifdef COMPILER_BUILD
#include "compiler/ctranslator/CTranslator.h"
#include "core/source/LocationManager.h"

#endif

#ifdef DEBUG
#define DEBUG_FUTURES true
#endif

std::ostream& operator<<(std::ostream& os, const LabModule& mod) {
    if(mod.scope_name.empty()) {
        os << mod.name;
    } else {
        os << mod.scope_name << ':' << mod.name;
    }
    return os;
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
    for(auto nested : file->get_dependencies()) {
        recursive_dedupe(nested, imported, flat_map);
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
std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<LabModule*>& modules) {
    std::vector<LabModule*> new_modules;
    std::unordered_map<LabModule*, bool> imported;
    for(auto mod : modules) {
        recursive_dedupe(mod, imported, new_modules);
    }
    return new_modules;
}

LabBuildCompiler::LabBuildCompiler(
    LocationManager& loc_man,
    CompilerBinder& binder,
    LabBuildCompilerOptions *options
) : path_handler(options->exe_path), loc_man(loc_man), binder(binder), options(options), pool((int) std::thread::hardware_concurrency()),
    global_allocator(100000 /** 100 kb**/), type_builder(global_allocator)
{

}

int LabBuildCompiler::do_job(LabJob* job) {
    job->status = LabJobStatus::Launched;
    int return_int;
    switch(job->type) {
        case LabJobType::Executable:
            return_int = do_executable_job(job);
            break;
        case LabJobType::Library:
            return_int = do_library_job(job);
            break;
        case LabJobType::ToCTranslation:
        case LabJobType::ProcessingOnly:
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
    return return_int;
}

inline void print_results(ASTFileResult& result, const std::string& abs_path, bool benchmark) {
    ASTProcessor::print_results(result, chem::string_view(abs_path), benchmark);
}

typedef void(*ImportCycleHandler)(void* data_ptr, std::vector<unsigned int>& parents, ASTFileResult* imported_file, ASTFileResult* parent, bool direct);

void check_imports_for_cycles(void* data_ptr, ASTFileResult* parent_file, std::vector<unsigned int>& parents, ImportCycleHandler handler) {
    auto end_size = parents.size();
    for(const auto imported : parent_file->imports) {
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

void flatten(std::vector<ASTFileResult*>& flat_out, std::unordered_map<std::string_view, bool>& done_files, ASTFileResult* single_file) {
    for(auto& file : single_file->imports) {
        flatten(flat_out, done_files, file);
    }
    auto view = std::string_view(single_file->abs_path);
    auto found = done_files.find(view);
    if(found == done_files.end()) {
        done_files[view] = true;
        flat_out.emplace_back(single_file);
    }
}

std::vector<ASTFileResult*> flatten(const std::span<ASTFileResult*>& files) {
    std::vector<ASTFileResult*> flat_out;
    std::unordered_map<std::string_view, bool> done_files;
    for(auto file : files) {
        flatten(flat_out, done_files, file);
    }
    return flat_out;
}

namespace fs = std::filesystem;

bool copyFile(const fs::path& sourcePath, const fs::path& destinationPath) {
    try {
        fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return false;
    }
}

bool determine_change_in_files(LabBuildCompiler* compiler, LabModule* mod, const std::string& mod_timestamp_file) {

    auto& direct_files = mod->direct_files;
    const auto verbose = compiler->options->verbose;
    const auto caching = compiler->options->is_caching_enabled;

    if(!caching) {
        if(verbose) {
            std::cout << "[lab] " << "skipping cache use, caching disabled" << std::endl;
        }
        return true;
    }

    if(verbose) {
        std::cout << "[lab] " << "checking if module " << mod->scope_name << ':' << mod->name << " has changed" << std::endl;
    }

    if(fs::exists(mod->object_path.to_view())) {

        if (verbose) {
            std::cout << "[lab] " << "found cached object file '" << mod->object_path << "', checking timestamp" << std::endl;
        }

        if(mod->type == LabModuleType::CFile || mod->type == LabModuleType::CPPFile) {

            std::vector<std::string_view> paths;
            for(auto& path : mod->paths) {
                paths.emplace_back(path.to_view());
            }

            // let's check if module timestamp file exists and is valid (files haven't changed)
            if (compare_mod_timestamp(paths, mod_timestamp_file)) {

                if (verbose) {

                    std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "', reusing" << std::endl;

                }

                return false;

            } else if (verbose) {

                std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;

            }

        } else {

            // let's check if module timestamp file exists and is valid (files haven't changed)
            if (compare_mod_timestamp(direct_files, mod_timestamp_file)) {

                if (verbose) {

                    std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "', reusing" << std::endl;

                }

                return false;

            } else if (verbose) {

                std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;

            }

        }

    } else if(verbose) {

        std::cout << "[lab] " << "couldn't find cached object file at '" << mod->object_path << "' for module '" << mod->scope_name << ':' << mod->name << std::endl;

    }

    return true;

}

std::string get_mod_timestamp_path(const std::string_view& build_dir, LabModule* mod, bool use_tcc) {
    auto f = mod->format('.');
    f.append(use_tcc ? "/timestamp_tcc.dat" : "/timestamp.dat");
    return resolve_rel_child_path_str(build_dir, f);
}

bool has_module_changed(LabBuildCompiler* compiler, LabModule* module, const std::string& build_dir, bool use_tcc) {
    bool has_deps_changed = false;
    for(const auto dep : module->dependencies) {
        if(has_module_changed(compiler, dep, build_dir, use_tcc)) {
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

bool determine_if_files_have_changed(LabBuildCompiler* compiler, const std::vector<ASTFileResult*>& files, const std::string_view& object_path, const std::string& mod_timestamp_file) {

    const auto verbose = compiler->options->verbose;

    if(fs::exists(object_path)) {

        if (verbose) {
            std::cout << "[lab] " << "found cached object file '" << object_path << "', checking timestamp" << std::endl;
        }

        // let's check if module timestamp file exists and is valid (files haven't changed)
        if (compare_mod_timestamp(files, mod_timestamp_file)) {

            if (verbose) {

                std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "', reusing" << std::endl;

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
                    user[func] = nullptr;
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

void process_cached_module(ASTProcessor& processor, std::vector<ASTFileMetaData>& files) {
    for(const auto& file : files) {
        auto& nodes = file.result->unit.scope.body.nodes;
        for(const auto node : nodes) {
            set_generated_instantiations(node);
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

int LabBuildCompiler::process_module_tcc(
        LabModule* mod,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        const std::string& mod_timestamp_file,
        const std::string& out_c_file,
        bool do_compile,
        std::stringstream& output_ptr
) {

    // variables
    const auto caching = options->is_caching_enabled;
    const auto verbose = options->verbose;
    const bool is_use_obj_format = options->use_mod_obj_format;

    auto& resolver = *processor.resolver;

    // direct files are stored inside the module
    auto& direct_files = mod->direct_files;

    // this would import these direct files (lex and parse), into the module files
    // the module files will have imports, any file imported (from this module or external module will be included)
    const auto parse_success = processor.import_module_files_direct(pool, direct_files, mod);

    // lets print diagnostics for all files in module
    for(auto& file : direct_files) {
        print_results(*file.result, file.abs_path, options->benchmark);
    }

    // return failure if parse failed
    if(!parse_success) {
        return 1;
    }

    const auto mod_data_path = is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data();
    if(mod_data_path && do_compile) {
        std::cout << rang::bg::gray << rang::fg::black << "[lab] " << "Building module ";
        if (!mod->name.empty()) {
            std::cout << '\'' << mod->name.data() << "' ";
        }
        std::cout << "at path '" << mod_data_path << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
    }

    // clear the current c string
    // some modules cause errors, don't compile, previous module may even have half contents
    // so we must clean that before beginning
    output_ptr.clear();
    output_ptr.str("");

    // preparing translation
    c_visitor.prepare_translate();

    if(verbose) {
        std::cout << "[lab] " << "resolving symbols in the module" << std::endl;
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = processor.sym_res_module(mod);
    if(sym_res_status == 1) {
        return 1;
    }

    // check if module has not changed, and use cache appropriately
    // not changed means object file is also present (we make the check when setting the boolean)
    if(mod->has_changed.has_value() && !mod->has_changed.value()) {

        if(verbose) {
            std::cout << "[lab] " << "module " << mod->scope_name << ':' << mod->name << " hasn't changed, skipping compilation" << std::endl;
        }

        // this will set all the generic instantiations to generated
        // which means generic decls won't generate those instantiations
        process_cached_module(processor, mod->direct_files);
        for(const auto dep : mod->dependencies) {
            process_cached_module(processor, dep->direct_files);
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
    processor.translate_module(
            c_visitor, mod
    );

    if(verbose) {
        std::cout << "[lab] " << "disposing non-public symbols in the module" << std::endl;
    }

    // removing non public nodes, because these would be disposed when allocator clears
    remove_non_public_nodes(processor, mod->direct_files);

    // disposing data
    mod_allocator->clear();

    // getting the c program
    const auto& program = output_ptr.str();

    // writing the translated c file (if user required)
    if(!out_c_file.empty()) {
        if(mod->type == LabModuleType::CFile) {
            copyFile(mod->paths[0].to_view(), out_c_file);
        } else {
            writeToFile(out_c_file, program);
        }
    } else {
        // TODO place a check here
        const auto out_path = resolve_sibling(mod->object_path.to_view(), mod->name.to_std_string() + ".2c.c");
        writeToFile(out_path, program);
    }

    // compiling the c program, if required
    if(do_compile) {
        auto obj_path = mod->object_path.to_std_string();
        if(verbose) {
            std::cout << "[lab] emitting the module '" << mod->name <<  "' object file at path '" << obj_path << '\'' << std::endl;
        }
        const auto compile_c_result = compile_c_string(options->exe_path.data(), program.c_str(), obj_path, false, options->benchmark, options->outMode == OutputMode::DebugComplete);
        if (compile_c_result == 1) {
            const auto out_path = resolve_sibling(mod->object_path.to_view(), mod->name.to_std_string() + ".debug.c");
            writeToFile(out_path, program);
            std::cerr << rang::fg::red << "[lab] couldn't build module '" << mod->name.data() << "' due to error in translation, translated C written at " << out_path << rang::fg::reset << std::endl;
            return 1;
        }
        // exe->linkables.emplace_back(obj_path);
        if(caching) {
            save_mod_timestamp(direct_files, mod_timestamp_file);
        }
    }

    // clear the current c string
    output_ptr.clear();
    output_ptr.str("");

    return 0;

}

#ifdef COMPILER_BUILD

int LabBuildCompiler::process_module_gen(
        LabModule* mod,
        ASTProcessor& processor,
        Codegen& gen,
        CTranslator& cTranslator,
        const std::string& mod_timestamp_file
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

    // lets print diagnostics for all files in module
    for(auto& file : direct_files) {
        print_results(*file.result, file.abs_path, options->benchmark);
    }

    // return failure if parse failed
    if(!parse_success) {
        return 1;
    }

    const auto mod_data_path = is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data();
    if(mod_data_path) {
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
    gen.module_init(mod->scope_name.to_chem_view(), mod->name.to_chem_view());

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
    if(sym_res_status == 1) {
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
        process_cached_module(processor, mod->direct_files);
        for(const auto dep : mod->dependencies) {
            process_cached_module(processor, dep->direct_files);
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
    configure_emitter_opts(options->outMode, &emitter_options);
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

    const auto gen_path = is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data();
    if(verbose) {
        std::cout << "[lab] emitting the module '" << mod->name << "' at '" << gen_path << '\'' << std::endl;
    }

    // creating a object or bitcode file
    const bool save_result = gen.save_with_options(&emitter_options);
    if(save_result) {
        if(gen_path) {
            if(caching) {
                save_mod_timestamp(direct_files, mod_timestamp_file);
            }
        }
    } else {
        std::cerr << "[lab] failed to emit file " << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data()) << " " << std::endl;
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
        case LabJobType::Library:
            std::cout << "Building library";
            break;
        case LabJobType::ToCTranslation:
            std::cout << "Translating to c";
            break;
        case LabJobType::ToChemicalTranslation:
            std::cout << "Translating to chemical";
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

void create_or_rebind_container(LabBuildCompiler* compiler, GlobalInterpretScope& global, SymbolResolver& resolver) {
    const auto verbose = compiler->options->verbose;
    if(compiler->container) {
        if(verbose) {
            std::cout << "[lab] " << "rebinding comptime methods" << std::endl;
        }
        // bind global container that contains namespaces like std and compiler
        // reusing it, if we created it before
        global.rebind_container(resolver, compiler->container);
    } else {
        if(verbose) {
            std::cout << "[lab] " << "creating comptime methods" << std::endl;
        }
        // allow user the compiler (namespace) functions in @comptime
        // we create the new global container here once
        compiler->container = global.create_container(resolver);
        if(verbose) {
            std::cout << "[lab] " << "created the global container" << std::endl;
        }
    }
}

int compile_c_or_cpp_module(LabBuildCompiler* compiler, LabModule* mod, const std::string& mod_timestamp_file) {
#ifndef COMPILER_BUILD
    if(mod->type == LabModuleType::CPPFile) {
        std::cerr << rang::fg::yellow << "[lab] skipping compilation of C++ module '" << *mod << '\'' << rang::fg::reset << std::endl;
        return 2;
    }
#endif
    const auto is_use_obj_format = compiler->options->use_mod_obj_format;
    std::cout << rang::bg::gray << rang::fg::black << "[lab] ";
    if(mod->type == LabModuleType::CFile) {
        std::cout << "compiling c ";
    } else {
        std::cout << "compiling c++ ";
    }
    if (!mod->name.empty()) {
        std::cout << '\'' << mod->name.data() << "' ";
    }
    std::cout << "at path '" << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data()) << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
#ifdef COMPILER_BUILD
    const auto compile_result = compile_c_file_to_object(mod->paths[0].data(), mod->object_path.data(), compiler->options->exe_path, {});
    if (compile_result == 1) {
        return 1;
    }
#else
    if(mod->type == LabModuleType::CPPFile) {
        std::cerr << rang::fg::yellow << "[lab] skipping compilation of C++ module '" << *mod << '\'' << rang::fg::reset << std::endl;
        return 1;
    }
    const auto compile_result = compile_c_file(compiler->options->exe_path.data(), mod->paths[0].data(), mod->object_path.to_std_string(), false, false, false);
    if (compile_result == 1) {
        return 1;
    }
#endif
    std::vector<std::string_view> paths;
    for(auto& path : mod->paths) {
        paths.emplace_back(path.to_view());
    }
    save_mod_timestamp(paths, mod_timestamp_file);
    return 0;
}

void create_mod_dir(LabBuildCompiler* compiler, LabJobType job_type, const std::string_view& build_dir, LabModule* mod) {
    const auto verbose = compiler->options->verbose;
    const auto use_tcc = compiler->use_tcc(job_type);
    const auto is_use_obj_format = use_tcc || compiler->options->use_mod_obj_format || mod->type == LabModuleType::CFile;
    // creating the module directory
    auto module_dir_path = resolve_rel_child_path_str(build_dir, mod->format('.'));
    auto mod_obj_path = resolve_rel_child_path_str(module_dir_path, (is_use_obj_format ? use_tcc ? "object_tcc.o" : "object.o" : "object.bc"));
    if (!module_dir_path.empty() && job_type != LabJobType::ToCTranslation) {
        const auto mod_dir_exists = fs::exists(module_dir_path);
        if (!mod_dir_exists) {
            if (verbose) {
                std::cout << "[lab] " << "creating module directory at path '" << module_dir_path << "'" << std::endl;
            }
            fs::create_directory(module_dir_path);
        }
    }
    if (job_type == LabJobType::Executable || job_type == LabJobType::CBI || job_type == LabJobType::ProcessingOnly ||
        job_type == LabJobType::Library) {
        if (is_use_obj_format) {
            mod->object_path.clear();
            mod->object_path.append(mod_obj_path);
        } else {
            mod->bitcode_path.clear();
            mod->bitcode_path.append(mod_obj_path);
        }
    }
}

inline void create_dir(const std::string& build_dir) {
    // create the build directory for this executable
    if (!std::filesystem::exists(build_dir)) {
        std::filesystem::create_directory(build_dir);
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

inline void create_job_build_dir(LabBuildCompiler* compiler, LabJob* job) {
    create_job_build_dir(compiler->options->verbose, job->build_dir.to_std_string());
}

int LabBuildCompiler::link_cbi_job(LabJobCBI* cbiJob, std::vector<LabModule*>& dependencies) {

    auto& job_name = cbiJob->name;
    auto cbiName = cbiJob->name.to_std_string();
    auto& cbiData = binder.data[cbiName];
    auto& outModDependencies = dependencies;

    const auto state = setup_tcc_state(options->exe_path.data(), "", true, options->outMode == OutputMode::DebugComplete);
    if(state == nullptr) {
        std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset;
        std::cerr << "couldn't create tcc state for jit of cbi '" << job_name << '\'' << std::endl;
        return 1;
    }

    // add module object files
    for(const auto dep : outModDependencies) {
        if(tcc_add_file(state, dep->object_path.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to add module '" << dep->scope_name << ':' << dep->name <<  "' in compilation of cbi '" << job_name << '\'' << std::endl;
            tcc_delete(state);
            return 1;
        }
    }

    // prepare for jit
    prepare_tcc_state_for_jit(state);

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

    // we compile the entirety of this module and store it
    // here putting this module in cbi is what will delete it
    // this is very important, otherwise tcc_delete won't be called on it
    cbiData.module = state;

    // error out if cbi types are empty
    if(cbiJob->indexes.empty()) {
        std::cerr << "[lab] " << rang::fg::red <<  "error: " << rang::fg::reset << "cbi job has no cbi types'" << job_name << '\'' << std::endl;
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

    const auto caching = options->is_caching_enabled;
    const auto verbose = options->verbose;

    begin_job_print(job);

    create_job_build_dir(this, job);

    if(verbose) {
        std::cout << "[lab] " << "allocating instances objects required for building" << std::endl;
    }

    // an interpretation scope for interpreting compile time function calls
    GlobalInterpretScope global(options->outMode, options->target_triple, nullptr, this, *job_allocator, type_builder, loc_man);

    // we hold the instantiated types inside this container
    InstantiationsContainer instContainer;

    // a new symbol resolver for every executable
    SymbolResolver resolver(binder, global, path_handler, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(binder, global, mangler, &output_ptr, *file_allocator, loc_man, options->debug_info);
    ToCBackendContext c_context(&c_visitor);
    global.backend_context = (BackendContext*) &c_context;

    // the processor we use
    ASTProcessor processor(path_handler, options, mod_storage, loc_man, &resolver, binder, type_builder, *job_allocator, *mod_allocator, *file_allocator);

    // import executable path aliases
    processor.path_handler.path_aliases = std::move(exe->path_aliases);

    create_or_rebind_container(this, global, resolver);

    // configure output path
    const bool is_use_obj_format = options->use_mod_obj_format;

    if(verbose) {
        std::cout << "[lab] " << "flattening the module structure" << std::endl;
    }

    // flatten the dependencies
    auto dependencies = flatten_dedupe_sorted(exe->dependencies);

    // index the modules, so imports can be resolved
    mod_storage.index_modules(dependencies);

    // allocating required variables before going into loop
    bool do_compile = job_type != LabJobType::ToCTranslation;

    // build dir
    auto build_dir = exe->build_dir.to_std_string();

    // for each module, let's determine its files and whether it has changed
    for(const auto mod : dependencies) {

        // determining module's direct files
        processor.determine_module_files(mod);

        // we must recalculate whether module's files have changed
        mod->has_changed = std::nullopt;

        // creating the module directory and getting the timestamp file path
        create_mod_dir(this, exe->type, build_dir, mod);

    }

    // if not a single module has changed, we consider it true
    bool has_any_changed = false;

    // now we check which module trees have changed
    for(const auto mod : exe->dependencies) {
        auto has_changed = has_module_changed(this, mod, build_dir, use_tcc(job));
        if(has_changed) {
            has_any_changed = true;
        }
    }

    if(!has_any_changed && do_compile) {

        // NOTE: there exists not a single module that has changed
        // which means we can safely link the previous object files again
        // we need to return early so modules won't be parsed at all

        for(const auto mod : dependencies) {
            job->linkables.emplace_back(mod->object_path.to_chem_view());
        }

        if(get_job_type == LabJobType::CBI) {
            const auto cbiJob = (LabJobCBI*) job;
            const auto jobDone = link_cbi_job(cbiJob, dependencies);
            if(jobDone != 0) {
                return jobDone;
            }
        }

        job->path_aliases = std::move(processor.path_handler.path_aliases);
        return 0;

    }

    // compile dependencies modules for this executable
    for(auto mod : dependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing module " << mod->scope_name << ':' << mod->name << std::endl;
        }

        // creating the module directory and getting the timestamp file path
        const auto mod_timestamp_file = get_mod_timestamp_path(build_dir, mod, use_tcc(job));

        if(do_compile) {
            switch (mod->type) {
                case LabModuleType::CPPFile:
                case LabModuleType::CFile: {
                    if(!mod->has_changed.has_value() || mod->has_changed.value()) {
                        const auto c_res = compile_c_or_cpp_module(this, mod, mod_timestamp_file);
                        if (c_res == 0) {
                            job->linkables.emplace_back(mod->object_path.copy());
                            continue;
                        } else if(c_res == 2) {
                            continue;
                        } else {
                            return 1;
                        }
                    } else {
                        job->linkables.emplace_back(mod->object_path.copy());
                        continue;
                    }
                }
                case LabModuleType::ObjFile:
                    for(auto& path : mod->paths) {
                        exe->linkables.emplace_back(path.copy());
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


        // figuring out the translated c output for the module (if user required)
        std::string out_c_file;
        if(get_job_type == LabJobType::ToCTranslation || !mod->out_c_path.empty()) {
            out_c_file = mod->out_c_path.to_std_string();
            if(out_c_file.empty()) {
                if(!job->build_dir.empty()) {
                    // TODO send this to module directory
                    out_c_file = resolve_rel_child_path_str(job->build_dir.to_view(), mod->name.to_std_string() + ".2c.c");
                } else if(!job->abs_path.empty()) {
                    out_c_file = job->abs_path.to_std_string();
                }
#ifdef DEBUG
                else {
                    throw std::runtime_error("couldn't figure out the output c path");
                }
#endif
            }
        }

        const auto result = process_module_tcc(mod, processor, c_visitor, mod_timestamp_file, out_c_file, do_compile, output_ptr);
        if(result == 1) {
            return 1;
        }

        if(do_compile) {
            job->linkables.emplace_back(mod->object_path.copy());
        }

    }

    if(get_job_type == LabJobType::CBI) {
        const auto cbiJob = (LabJobCBI*) job;
        const auto jobDone = link_cbi_job(cbiJob, dependencies);
        if(jobDone != 0) {
            return jobDone;
        }
    }

    exe->path_aliases = std::move(processor.path_handler.path_aliases);
    return 0;

}

#ifdef COMPILER_BUILD

int LabBuildCompiler::process_job_gen(LabJob* job) {

    const auto job_type = job->type;

    const auto caching = options->is_caching_enabled;
    const auto verbose = options->verbose;

    begin_job_print(job);

    create_job_build_dir(this, job);

    if(verbose) {
        std::cout << "[lab] " << "allocating instances objects required for building" << std::endl;
    }

    // an interpretation scope for interpreting compile time function calls
    GlobalInterpretScope global(options->outMode, options->target_triple, nullptr, this, *job_allocator, type_builder, loc_man);

    // generic instantiations types are stored here
    InstantiationsContainer instContainer;

    // a new symbol resolver for every executable
    SymbolResolver resolver(binder, global, path_handler, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    auto& job_alloc = *job_allocator;
    // a single c translator across this entire job
    CTranslator cTranslator(job_alloc, type_builder, options->is64Bit);
    ASTProcessor processor(path_handler, options, mod_storage, loc_man, &resolver, binder, type_builder, job_alloc, *mod_allocator, *file_allocator);
    CodegenOptions code_gen_options;
    if(cmd) {
        code_gen_options.fno_unwind_tables = cmd->has_value("", "fno-unwind-tables");
        code_gen_options.fno_asynchronous_unwind_tables = cmd->has_value("", "fno-asynchronous-unwind-tables");
        code_gen_options.no_pie = cmd->has_value("no-pie", "no-pie");
    }
    Codegen gen(code_gen_options, binder, global, mangler, options->target_triple, options->exe_path, options->is64Bit, options->debug_info, *file_allocator);
    LLVMBackendContext g_context(&gen);
    // set the context so compile time calls are sent to it
    global.backend_context = (BackendContext*) &g_context;

    // import executable path aliases
    processor.path_handler.path_aliases = std::move(job->path_aliases);

    create_or_rebind_container(this, global, resolver);

    // configure output path
    const bool is_use_obj_format = options->use_mod_obj_format;

    if(verbose) {
        std::cout << "[lab] " << "flattening the module structure" << std::endl;
    }

    // flatten the dependencies
    auto dependencies = flatten_dedupe_sorted(job->dependencies);

    // index the modules, so imports can be resolved
    mod_storage.index_modules(dependencies);

    // build dir
    auto build_dir = job->build_dir.to_std_string();

    // for each module, let's determine its files and whether it has changed
    for(const auto mod : dependencies) {

        // determining module's direct files
        processor.determine_module_files(mod);

        // we must recalculate which files have changed
        mod->has_changed = std::nullopt;

        // creating the module directory and getting the timestamp file path
        create_mod_dir(this, job->type, build_dir, mod);

    }

    // if not a single module has changed, we consider it true
    bool has_any_changed = false;

    for(const auto mod : job->dependencies) {
        const auto changed = has_module_changed(this, mod, build_dir, use_tcc(job));
        if(changed) {
            has_any_changed = true;
        }
    }

    if(!has_any_changed) {

        // NOTE: there exists not a single module that has changed
        // which means we can safely link the previous object files again
        // we need to return early so modules won't be parsed at all

        for(const auto mod : dependencies) {
            job->linkables.emplace_back(mod->object_path.to_chem_view());
        }

        job->path_aliases = std::move(processor.path_handler.path_aliases);
        return 0;

    }

    // compile dependent modules for this executable
    for(auto mod : dependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing module '" << mod->name << '\'' << std::endl;
        }

        // get the timestamp file path
        const auto mod_timestamp_file = get_mod_timestamp_path(job->build_dir.to_std_string(), mod, use_tcc(job));

        // handle c and cpp file modules
        switch (mod->type) {
            case LabModuleType::CFile:
            case LabModuleType::CPPFile: {
                if(!mod->has_changed.has_value() || mod->has_changed.value()) {
                    const auto c_res = compile_c_or_cpp_module(this, mod, mod_timestamp_file);
                    if(c_res == 0) {
                        job->linkables.emplace_back(mod->object_path.copy());
                        continue;
                    } else {
                        return 1;
                    }
                } else {
                    job->linkables.emplace_back(mod->object_path.copy());
                    continue;
                }
            }
            case LabModuleType::ObjFile:
                job->linkables.emplace_back(mod->paths[0].copy());
                continue;
            default:
                break;
        }

        const auto result = process_module_gen(mod, processor, gen, cTranslator, mod_timestamp_file);
        if(result == 1) {
            return 1;
        }

        job->linkables.emplace_back(mod->object_path.copy());

    }

    job->path_aliases = std::move(processor.path_handler.path_aliases);

    return 0;

}

#endif

int link_objects(
    const std::string& comp_exe_path,
    std::vector<chem::string>& linkables,
    const std::string& output_path,
    const std::vector<std::string>& flags,
    const std::string_view& target_triple,
    bool use_tcc
) {
    int link_result;
#ifdef COMPILER_BUILD
    if(use_tcc) {
        chem::string copy(comp_exe_path);
        link_result = tcc_link_objects(copy.mutable_data(), output_path, linkables);
    } else {
        std::vector<std::string> data;
        for (auto& obj: linkables) {
            data.emplace_back(obj.data());
        }
        link_result = link_objects(data, output_path, comp_exe_path, flags, target_triple);
    }
#else
    chem::string copy(comp_exe_path);
    link_result = tcc_link_objects(copy.mutable_data(), output_path, linkables);
#endif
    if(link_result == 1) {
        std::cerr << "Failed to link \n";
        for(auto& linkable : linkables) {
            std::cerr << '\t' << linkable << '\n';
        }
        std::cerr << "into " << output_path;
        std::cerr << std::endl;
        return link_result;
    }

    return link_result;
}

int LabBuildCompiler::link(std::vector<chem::string>& linkables, const std::string& output_path, bool use_tcc) {
    std::vector<std::string> flags;
#ifdef COMPILER_BUILD
    if(options->no_pie && !use_tcc) {
        flags.emplace_back("-no-pie");
    }
#endif
    if(options->debug_info) {
        flags.emplace_back("-g");
        // TODO so on windows -gdward-4 is being used as .pdb and .ilk are being generated which aren't supported by gdb
//        flags.emplace_back("-gdwarf-4");
    }
    if(options->verbose) {
        std::cout << "[lab] linking objects ";
        for(auto& obj : linkables) {
            std::cout << '\'' << obj.to_view() << '\'' << ' ';
        }
        std::cout << "with flags ";
        for(auto& str : flags) {
            std::cout << '\'' << str << '\'' << ' ';
        }
        std::cout << std::endl;
        flags.emplace_back("-v");
    }
    return link_objects(options->exe_path, linkables, output_path, flags, options->target_triple, use_tcc);
}

int LabBuildCompiler::do_executable_job(LabJob* job) {
    auto result = process_modules(job);
    if(result == 1) {
        return 1;
    }
    // link will automatically detect the extension at the end
    return link(job->linkables, job->abs_path.to_std_string(), use_tcc(job));
}

int LabBuildCompiler::do_library_job(LabJob* job) {
    auto result = process_modules(job);
    if(result == 1) {
        return 1;
    }
    // link will automatically detect the extension at the end
    return link(job->linkables, job->abs_path.to_std_string(), use_tcc(job));
}

int LabBuildCompiler::do_to_chemical_job(LabJob* job) {
#ifdef COMPILER_BUILD
    std::ofstream output;
    output.open(job->abs_path.data());
    if(!output.is_open()) {
        std::cerr << rang::fg::red << "[lab] " << "couldn't open the file for writing translated chemical from c at '" << job->abs_path.data() << '\'' << rang::fg::reset << std::endl;
        return 1;
    }
    for(auto mod : job->dependencies) {
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
        ModuleDependencyRecord& dependency
) {

    auto& module_path = dependency.module_dir_path;
    auto buildLabPath = resolve_rel_child_path_str(module_path, "build.lab");
    if(std::filesystem::exists(buildLabPath)) {

        // check if we have already parsed this build.lab (from another module's dependency)
        auto found = buildLabDependenciesCache.find(buildLabPath);
        if(found != buildLabDependenciesCache.end()) {
            return found->second;
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
        auto build = (LabModule*(*)(LabBuildContext*)) tcc_get_symbol(state, "chemical_lab_build");
        if(!build) {
            std::cerr << rang::fg::red << "[lab] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
            return nullptr;
        }

        // call the root build.lab build's function
        const auto modPtr = build(&context);

        // store the mod pointer in cache, so we don't need to build this build.lab again
        buildLabDependenciesCache[std::move(buildLabPath)] = modPtr;

        return modPtr;

    } else {

        const auto modFilePath = resolve_rel_child_path_str(module_path, "chemical.mod");
        if(std::filesystem::exists(modFilePath)) {

            return built_mod_file(context, modFilePath);

        } else {

            std::cerr << rang::fg::red << "error:" << rang::fg::reset << " directory at path '" << dependency.module_dir_path << "' doesn't contain a 'build.lab' or 'chemical.mod' therefore cannot be imported" << std::endl;
            return nullptr;

        }


    };

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
    auto modFileId = loc_man.encodeFile(modFilePath.str());
    ModuleFileData modFileData(modFileId, modFilePath);

    // set those module names
    modFileData.scope_name = scope_name;
    modFileData.module_name = module_name;

    // import the file into result (lex and parse)
    const auto isModFileOk = ASTProcessor::import_chemical_mod_file(allocator, allocator, loc_man, modFileData, modFileId, modFilePath.view());

    // TODO check if mod file is ok and report errors appropriately

    // opening the file
    std::ofstream stream;
    stream.open(outputPath.data());
    if(!stream.is_open()) {
        // TODO report error
    }

    // actual conversion
    convertToBuildLab(modFileData, stream);

    // closing the writing stream
    stream.close();
    return 0;


}

static bool neg_it(bool neg_flag, bool result) {
    return neg_flag ? !result : result;
}

LabModule* LabBuildCompiler::build_module_from_mod_file(
        LabBuildContext& context,
        const std::string_view& modFilePathView
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

    if(errorMsg == nullptr) {
        const auto module = context.storage.find_module(scope_name, module_name);
        if(module != nullptr) {
            return module;
        }
    } else {
        std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't get module declaration from the mod file at '" << modFilePathView << "' because of error '" << errorMsg << '\'' << std::endl;
    }

    if(verbose) {
        std::cout << "[lab] " << "parsing mod file '" << modFilePathView << '\'' << std::endl;
    }

    std::string modFilePath(modFilePathView);

    // module file data
    auto modFilePathChemView = chem::string_view(modFilePathView);
    auto modFileId = loc_man.encodeFile(modFilePath);
    ModuleFileData modFileData(modFileId, modFilePathChemView);

    // set those module names
    modFileData.scope_name = scope_name;
    modFileData.module_name = module_name;

    // import the file into result (lex and parse)
    const auto isModFileOk = ASTProcessor::import_chemical_mod_file(*file_allocator, *mod_allocator, loc_man, modFileData, modFileId, modFilePath);

    // printing the diagnostics for the file
    Diagnoser::print_diagnostics(modFileData.diagnostics, modFilePathChemView, "Parser");

    // error out if not ok
    if (!isModFileOk) {
        std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't parse the mod file at '" << modFilePathView << "' due to errors" << std::endl;
        return nullptr;
    }

    // create a new module
    const auto module = context.new_module(scope_name, module_name);

    // get all the sources
    for(auto& src : modFileData.sources_list) {
        if(src.if_condition.empty()) {
            module->paths.emplace_back(resolve_sibling(modFilePathView, src.path.view()));
        } else {
            const auto cond_result = is_condition_enabled(container, src.if_condition);
            if(cond_result.has_value()) {
                if(neg_it(src.is_negative, cond_result.value())) {
                    module->paths.emplace_back(resolve_sibling(modFilePathView, src.path.view()));
                }
            } else {
                std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "unknown condition '" << src.if_condition << "'" << std::endl;
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

    if (verbose) {
        std::cout << "[lab] " << "created module for '" << module->scope_name << ':' << module->name << "'" << std::endl;
    }

    // module dependencies we determined from directly imported files
    std::vector<ModuleDependencyRecord> buildLabModuleDependencies;

    // this function figures out dependencies based on import statements
    path_handler.figure_out_mod_dep_using_imports(
            modFilePathView,
            buildLabModuleDependencies,
            modFileData.scope.body.nodes,
            container
    );

    // these are modules imported by the build.lab
    // however we must build their build.lab or chemical.mod into a LabModule*
    for(auto& mod_ptr : buildLabModuleDependencies) {
        // get the module pointer
        const auto modDependency = create_module_for_dependency(context, mod_ptr);
        if(modDependency == nullptr) {
            return nullptr;
        }
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
        std::stringstream& output_ptr,
        bool mod_file_source
) {

    auto& lab_processor = processor;
    auto& lab_resolver = *processor.resolver;
    const auto verbose = options->verbose;

    if(verbose) {
        std::cout << "[lab] building lab file at '" << path_view << '\'' << std::endl;
    }

    LabModule chemical_lab_module(LabModuleType::Files, chem::string("chemical"), chem::string("lab"));
    std::string path(path_view);
    auto buildLabFileId = loc_man.encodeFile(path);
    ASTFileMetaData buildLabMetaData(buildLabFileId, &chemical_lab_module.module_scope, path, path, "");
    ASTFileResult labFileResult(buildLabFileId, path, &chemical_lab_module.module_scope);

    // import the file into result (lex and parse)
    // NOTE: we import these files on job allocator, because a build.lab has dependencies on modules
    // that we need to compile, which will free the module allocator, so if we kept on module allocator
    // we will lose everything after processing dependencies
    if(mod_file_source) {
        lab_processor.import_mod_file_as_lab(buildLabMetaData, labFileResult, true);
    } else {
        lab_processor.import_file(labFileResult, buildLabMetaData.file_id, buildLabMetaData.abs_path, true);
    }

    // printing results for module file parsing
    print_results(labFileResult, path, true);

    // probably an error during parsing
    if(!labFileResult.continue_processing) {
        if(verbose) {
            std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't import the 'build.lab' file at '" << path << "' due to errors" << std::endl;
        }
        return nullptr;
    }

    // figure out direct imported by this build.lab file
    auto& direct_files_in_lab = chemical_lab_module.direct_files;
    lab_processor.figure_out_direct_imports(buildLabMetaData, labFileResult.unit.scope.body.nodes, direct_files_in_lab);

    // if has imports, we import those files as well
    // it's required to build a proper import tree
    if(!direct_files_in_lab.empty()) {
        // NOTE: we import these files on job allocator, because a build.lab has dependencies on modules
        // that we need to compile, which will free the module allocator, so if we kept on module allocator
        // we will lose everything after processing dependencies
        const auto success = lab_processor.import_chemical_files_recursive(pool, labFileResult.imports, direct_files_in_lab, true);
        if(!success) {
            return nullptr;
        }
    }

    ASTFileResult* files_to_flatten[] = { &labFileResult };

    // check module files for import cycles (direct or indirect)
    ImportCycleCheckResult importCycle { false, loc_man };
    check_imports_for_cycles(importCycle, files_to_flatten);
    if(importCycle.has_cyclic_dependencies) {
        return nullptr;
    }

    // flatten the files to module files (in sorted order of independence)
    auto module_files = flatten(files_to_flatten);

    // we figure out all the files that belong to this build.lab module
    direct_files_in_lab.clear();
    for(const auto f : module_files) {
//        if(f->abs_path.ends_with(".lab")) {
            f->result = f;
            direct_files_in_lab.emplace_back(*f);
//        }
    }

    // the build lab object file (cached)
    const auto labDir = resolve_rel_child_path_str(options->build_dir, "lab");
    const auto labBuildDir = resolve_rel_child_path_str(labDir, "build");
    const auto labModDir = resolve_rel_child_path_str(labBuildDir, "chemical.lab");

    // create required directories
    create_dir(labDir);
    create_dir(labBuildDir);
    create_dir(labModDir);

    // figure out where to store the build lab object and dat file
    const auto buildLabObj = resolve_rel_child_path_str(labModDir, "build.lab.o");
    const auto buildLabTimestamp = resolve_rel_child_path_str(labModDir, "build.lab.dat");

    // determine if build lab has changed
    const auto has_buildLabChanged = determine_if_files_have_changed(this, module_files, buildLabObj, buildLabTimestamp);

    // module dependencies we determined from directly imported files
    std::vector<ModuleDependencyRecord> buildLabModuleDependencies;

    // based on imports figures out which modules have been imported
    path_handler.figure_out_mod_dep_using_imports(
        path_view,
        buildLabModuleDependencies,
        labFileResult.unit.scope.body.nodes,
        container
    );

    // direct module dependencies (in no valid order)
    auto& mod_dependencies = chemical_lab_module.dependencies;

    // these are modules imported by the build.lab
    // however we must build their build.lab or chemical.mod into a LabModule*
    for(auto& mod_ptr : buildLabModuleDependencies) {

        // get the module pointer
        const auto mod = create_module_for_dependency(context, mod_ptr);
        if(mod == nullptr) {
            return nullptr;
        }

        mod_dependencies.emplace_back(mod);

    }

    // including all (+nested) dependencies in a single vector
    // sorted in the order of least dependence (flattened with all the dependencies in one vector)
    auto outModDependencies = flatten_dedupe_sorted(mod_dependencies);

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

    // if not a single module has changed, we consider it true
    bool has_any_changed = false;

    // check which modules have changed
    for(const auto mod : mod_dependencies) {
        const auto changed = has_module_changed(this, mod, lab_mods_dir, true);
        if(changed) {
            has_any_changed = true;
        }
    }

    if(!has_any_changed && !has_buildLabChanged) {

        // NOTE: there exists not a single module that has changed
        // also not a single file in the build.lab has changed and its object file also exists
        // which means we can safely link the previous object files again

        const auto state = setup_tcc_state(options->exe_path.data(), "", true, is_debug(options->outMode));
        if(state == nullptr) {
            std::cerr << "[lab] " << rang::fg::red << "error: " << rang::fg::reset;
            std::cerr << "couldn't create tcc state for jit of cached build.lab object file" << std::endl;
            return nullptr;
        }

        // add module object files
        for(const auto dep : outModDependencies) {
            if(tcc_add_file(state, dep->object_path.data()) == -1) {
                std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to add module '" << dep->scope_name << ':' << dep->name <<  "' in compilation of cached 'build.lab'" << std::endl;
                tcc_delete(state);
                return nullptr;
            }
        }

        // add final object file (of the build.lab we cached earlier)
        if(tcc_add_file(state, buildLabObj.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error: " << rang::fg::reset << "failed to add file '" << buildLabObj <<  "' in compilation of cached 'build.lab'" << std::endl;
            tcc_delete(state);
            return nullptr;
        }

        // prepare for jit
        prepare_tcc_state_for_jit(state);

        // import all compiler interfaces the modules require
        for(const auto mod : outModDependencies) {
            for(auto& interface : mod->compiler_interfaces) {
                CompilerBinder::import_compiler_interface(interface, state);
            }
        }

        // relocate the code before calling
        if(tcc_relocate(state) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error: " << rang::fg::reset << "failed to relocate cached build.lab" << std::endl;
            tcc_delete(state);
            return nullptr;
        }

        return state;

    }

    // processing flattened dependencies
    for(const auto mod : outModDependencies) {

        // the timestamp file is what determines whether the module needs to be rebuilt again
        const auto timestamp_path = get_mod_timestamp_path(lab_mods_dir, mod, true);

        // the c output for this module, so we can debug
        const auto out_c_path = resolve_sibling(timestamp_path, "mod.2c.c");

        // compile the module
        const auto module_result = process_module_tcc(mod, processor, c_visitor, timestamp_path, out_c_path, true, output_ptr);
        if(module_result == 1) {
            return nullptr;
        }

        // since the job was successful, we can expect an object file at module's object_path
        // we'll use this object file by linking it with tcc
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = lab_processor.sym_res_module_seq(&chemical_lab_module);
    if(sym_res_status == 1) {
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

            if (file.as_identifier.empty()) {
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
                    throw std::runtime_error("integer conversion truncated or failed.");
                }
#endif

                // building the name string, for the build.lab which will be as_identifier + '_' + file_index
                const auto name_size = file.as_identifier.size() + total_written;
                const auto name_str = job_allocator->allocate_released_size(name_size + 1, 1); // 1 for the null terminator
                std::memcpy(name_str, file.as_identifier.data(), file.as_identifier.size());
                std::memcpy(name_str + file.as_identifier.size(), nameBuffer, static_cast<size_t>(total_written));
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

    // clear the output ptr
    output_ptr.clear();
    output_ptr.str("");

    // preparing translation
    c_visitor.prepare_translate();

    // translating the build.lab module
    lab_processor.translate_module(
        c_visitor, &chemical_lab_module
    );

    // compiling the c output from build.labs
    const auto& str = output_ptr.str();

    // TODO place a check to only output this when need be
    const auto labOutCPath = resolve_rel_child_path_str(labModDir, "build.lab.c");
    writeToFile(labOutCPath, str);

    // let's first create an object file for build.lab (for caching)
    const auto objRes = compile_c_string(options->exe_path.data(), str.data(), buildLabObj, false, false, options->outMode == OutputMode::DebugComplete);
    if(objRes == -1){

        std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to compile 'build.lab' at '" << path <<  "' to object file, written to '" << labOutCPath << '\'' << std::endl;

        // TODO: object file compilation failed, we must delete any existing timestamp file (so next time, we must build the build.lab from scratch)


    } else {

        // since object file compilation was successful, we should save a timestamp
        save_mod_timestamp(module_files, std::string_view(buildLabTimestamp));

    }

    // creating a new tcc state
    const auto state = setup_tcc_state(options->exe_path.data(), "", true, is_debug(options->outMode));
    if(state == nullptr) {
        std::cerr << "[lab] " << rang::fg::red << "error:" << rang::fg::reset << "couldn't create tcc state for 'build.lab' file at '" << path << "', written to '" << labOutCPath << '\'' << std::endl;
        return nullptr;
    }

    // add module object files
    for(const auto dep : outModDependencies) {
        if(tcc_add_file(state, dep->object_path.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to add module '" << dep->scope_name << ':' << dep->name <<  "' in compilation of 'build.lab'" << std::endl;
            tcc_delete(state);
            return nullptr;
        }
    }

    // compiling the program
    if(tcc_compile_string(state, str.data()) == -1) {
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

    // relocate the code before calling
    tcc_relocate(state);

    // clear the output ptr
    output_ptr.clear();
    output_ptr.str("");

    // clear the allocators
    mod_allocator->clear();
    file_allocator->clear();

    // return the state
    return state;

}

LabModule* LabBuildCompiler::built_mod_file(LabBuildContext& context, const std::string_view& path) {

    // create a directory for lab processing and dependent modules
    // we'll call it 'lab' inside the build directory
    const auto lab_dir = resolve_rel_child_path_str(options->build_dir, "lab");
    const auto lab_mods_dir = resolve_rel_child_path_str(lab_dir, "modules");

    // create lab dir and modules dir inside lab dir (lab, lab/modules)
    create_dir(lab_dir);
    create_dir(lab_mods_dir);

    // call the function
    const auto result = build_module_from_mod_file(
            context, path
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

    // the allocators that will be used for all jobs
    set_allocators(
            &_job_allocator,
            &_mod_allocator,
            &_file_allocator
    );

    // do the jobs
    const auto result = do_jobs(this, data);

    return result;

}

int LabBuildCompiler::do_job_allocating(LabJob* job) {

    return do_allocating((void*) job, [](LabBuildCompiler* compiler, void* data) {

        const auto job = (LabJob*) data;
        compiler->current_job = job;
        return compiler->do_job(job);

    });

}

TCCState* LabBuildCompiler::built_lab_file(
        LabBuildContext& context,
        const std::string_view& path,
        bool mod_file_source
) {

    // a global interpret scope required to evaluate compile time things
    GlobalInterpretScope global(options->outMode, options->target_triple, nullptr, this, *job_allocator, type_builder, loc_man);

    // instantiations container holds the types
    InstantiationsContainer instContainer;

    // creating symbol resolver for build.lab files only
    SymbolResolver lab_resolver(binder, global, path_handler, instContainer, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // the processor that does everything for build.lab files only
    ASTProcessor lab_processor(
            path_handler,
            options,
            mod_storage,
            loc_man,
            &lab_resolver,
            binder,
            type_builder,
            *job_allocator,
            *mod_allocator,
            *file_allocator
    );

    // creates or rebinds the global container
    create_or_rebind_container(this, global, lab_resolver);

    // compiler interfaces the lab files imports
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(binder, global, mangler, &output_ptr, *file_allocator, loc_man, options->debug_info);
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

    // get build lab file into a tcc state
    const auto state = built_lab_file(
            context, path, lab_processor, c_visitor, output_ptr, mod_file_source
    );

    return state;

}

int LabBuildCompiler::build_lab_file(LabBuildContext& context, const std::string_view& path) {

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // the allocators that will be used for all jobs
    job_allocator = &_job_allocator;
    mod_allocator = &_mod_allocator;
    file_allocator = &_file_allocator;

    // mkdir the build directory
    create_dir(options->build_dir);

    // get build lab file into a tcc state
    const auto state = built_lab_file(context, path, false);
    if(!state) {
        return 1;
    }

    // automatic destroy
    TCCDeletor auto_del(state);

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

        current_job = exe.get();

        job_result = do_job(exe.get());
        if(job_result == 1) {
            std::cerr << rang::fg::red << "[lab] " << "error performing job '" << exe->name.data() << "', returned status code 1" << rang::fg::reset << std::endl;
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

int LabBuildCompiler::build_mod_file(LabBuildContext& context, const std::string_view& path, chem::string outputPath) {

    // allocating ast allocators
    const auto job_mem_size = 100000; // 100 kb
    const auto mod_mem_size = 100000; // 100 kb
    const auto file_mem_size = 100000; // 100 kb
    ASTAllocator _job_allocator(job_mem_size);
    ASTAllocator _mod_allocator(mod_mem_size);
    ASTAllocator _file_allocator(file_mem_size);

    // the allocators that will be used for all jobs
    job_allocator = &_job_allocator;
    mod_allocator = &_mod_allocator;
    file_allocator = &_file_allocator;

    // mkdir the build directory
    create_dir(options->build_dir);

    // get build lab file into a tcc state
    const auto state = built_lab_file(context, path, true);
    if(!state) {
        return 1;
    }

    // automatic destroy
    TCCDeletor auto_del(state);

    // clear everything from allocators before proceeding
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    // get the build method
    auto build = (LabModule*(*)(LabBuildContext*)) tcc_get_symbol(state, "chemical_lab_build");
    if(!build) {
        std::cerr << rang::fg::red << "[lab] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
        return 1;
    }

    // clear the module storage
    // these modules were created to facilitate the build.lab generation
    // if not cleared, these modules will interfere with modules created for executable
    context.storage.clear();

    // call the root build.lab build's function
    const auto main_module = build(&context);

    // lets compile any cbi jobs user may have specified
    for(auto& job : context.executables) {
        if(job->type == LabJobType::CBI) {
            const auto job_result = do_job(job.get());
            if(job_result != 0) {
                return job_result;
            }
        }
    }

    // lets create a single job
    LabJob final_job(LabJobType::Executable, chem::string("main"), std::move(outputPath), chem::string(options->build_dir));

    // check if user gave command to output ir or asm
    if(cmd) {
        const auto has_ll = cmd->has_value("out-ll-all");
        const auto has_asm = cmd->has_value("out-asm-all");
        if(has_ll || has_asm) {
            for(auto& modPtr : mod_storage.get_modules()) {
                auto& module = *modPtr.get();
                const auto mod_dir = resolve_rel_child_path_str(final_job.build_dir.to_view(), module.name.to_view());
                if (has_ll) {
                    module.llvm_ir_path.append(resolve_rel_child_path_str(mod_dir, "llvm_ir.ll"));
                }
                if (has_asm) {
                    module.asm_path.append(resolve_rel_child_path_str(mod_dir, "mod_asm.s"));
                }
            }
        }
    }

    // just put all the modules as this job's dependency
    final_job.dependencies.emplace_back(main_module);

    current_job = &final_job;

    const auto job_result = do_job(&final_job);
    if(job_result == 1) {
        std::cerr << rang::fg::red << "[lab] " << "error emitting executable, returned status code 1" << rang::fg::reset << std::endl;
    }

    // clearing all allocations done in all the allocators
    _job_allocator.clear();
    _mod_allocator.clear();
    _file_allocator.clear();

    return job_result;

}

LabBuildCompiler::~LabBuildCompiler() {
    GlobalInterpretScope::dispose_container(container);
}