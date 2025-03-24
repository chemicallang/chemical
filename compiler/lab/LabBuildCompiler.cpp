// Copyright (c) Chemical Language Foundation 2025.

#include "rang.hpp"
#include "LabBuildCompiler.h"
#include "ast/types/LinkedType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/Import.h"
#include "ast/statements/PackageDefinition.h"
#include "ast/structures/If.h"
#include "utils/Benchmark.h"
#include "ast/structures/ModuleScope.h"
#include "Utils.h"
#include "cst/LocationManager.h"
#include <fstream>
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
#include "parser/model/CompilerBinder.h"
#include "compiler/SelfInvocation.h"
#include "utils/CmdUtils.h"

#ifdef COMPILER_BUILD
#include "compiler/ctranslator/CTranslator.h"
#include "cst/LocationManager.h"

#endif

#ifdef DEBUG
#define DEBUG_FUTURES true
#endif

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
    auto found = imported.find(file);
    if(found == imported.end()) {
        for(auto nested : file->dependencies) {
            recursive_dedupe(nested, imported, flat_map);
        }
        imported[file] = true;
        flat_map.emplace_back(file);
    }
}

/**
 * same as above, only it operates on multiple modules, it de-dupes the dependent modules
 * of the given list of modules and also sorts them
 * TODO
 * 1 - avoid direct cyclic dependencies a depends on b and b depends on a
 * 2 - avoid indirect cyclic dependencies a depends on b and b depends on c and c depends on a
 */
std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<LabModule*>& modules) {
    std::vector<LabModule*> new_modules;
    std::unordered_map<LabModule*, bool> imported;
    for(auto mod : modules) {
        recursive_dedupe(mod, imported, new_modules);
    }
    return new_modules;
}

/**
 * same as above, only it operates on multiple modules, it de-dupes the dependent modules
 * of the given list of modules and also sorts them
 * TODO
 * 1 - avoid direct cyclic dependencies a depends on b and b depends on a
 * 2 - avoid indirect cyclic dependencies a depends on b and b depends on c and c depends on a
 */
std::vector<LabModule*> flatten_dedupe_sorted(const std::vector<std::unique_ptr<LabModule>>& modules) {
    std::vector<LabModule*> new_modules;
    std::unordered_map<LabModule*, bool> imported;
    for(auto& mod : modules) {
        recursive_dedupe(mod.get(), imported, new_modules);
    }
    return new_modules;
}

namespace fs = std::filesystem;

/**
 * save mod timestamp data (modified date and file size) in a file that can be read later and compared
 * to check if files have changed
 */
void save_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& output_file) {
    std::ofstream ofs(output_file.data(), std::ios::binary);
    size_t num_files = files.size();
    ofs.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));
    for (const auto& file : files) {
        auto& file_abs_path = file.abs_path;
        fs::path file_path(file_abs_path);
        if (fs::exists(file_path)) {
            uintmax_t file_size = fs::file_size(file_path);
            auto mod_time = fs::last_write_time(file_path);
            size_t file_str_size = file_abs_path.size();
            ofs.write(reinterpret_cast<const char*>(&file_str_size), sizeof(file_str_size));
            ofs.write(file_abs_path.data(), (std::streamsize) file_str_size);
            ofs.write(reinterpret_cast<const char*>(&file_size), sizeof(file_size));
            ofs.write(reinterpret_cast<const char*>(&mod_time), sizeof(mod_time));
        }
    }
}

bool compare_mod_timestamp(const std::vector<ASTFileMetaData>& files, const std::string_view& prev_timestamp_file) {
    std::ifstream ifs(prev_timestamp_file.data(), std::ios::binary);

    if (!ifs.is_open()) {
        return false;
    }

    size_t prev_num_files;
    ifs.read(reinterpret_cast<char*>(&prev_num_files), sizeof(prev_num_files));

    if (prev_num_files != files.size()) {
        return false;
    }

    for (const auto& file : files) {
        fs::path file_path(file.abs_path);
        if (fs::exists(file_path)) {
            uintmax_t current_file_size = fs::file_size(file_path);
            auto current_mod_time = fs::last_write_time(file_path);

            size_t file_str_size;
            ifs.read(reinterpret_cast<char*>(&file_str_size), sizeof(file_str_size));

            std::string prev_file_str(file_str_size, '\0');
            ifs.read(&prev_file_str[0], (std::streamsize) file_str_size);

            uintmax_t prev_file_size;
            ifs.read(reinterpret_cast<char*>(&prev_file_size), sizeof(prev_file_size));

            fs::file_time_type prev_mod_time;
            ifs.read(reinterpret_cast<char*>(&prev_mod_time), sizeof(prev_mod_time));

            if (prev_file_str != file_path.string() || prev_file_size != current_file_size || prev_mod_time != current_mod_time) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

LabBuildCompiler::LabBuildCompiler(
    CompilerBinder& binder,
    LabBuildCompilerOptions *options
) : path_handler(options->exe_path), binder(binder), options(options), pool((int) std::thread::hardware_concurrency()) {

}

void LabBuildCompiler::prepare(
    ASTAllocator* const jobAllocator,
    ASTAllocator* const modAllocator,
    ASTAllocator* const fileAllocator
) {
    job_allocator = jobAllocator;
    mod_allocator = modAllocator;
    file_allocator = fileAllocator;
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

bool empty_diags(ASTFileResult& result) {
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty() && !result.lex_benchmark && !result.parse_benchmark;
}

void print_results(ASTFileResult& result, const std::string& abs_path, bool benchmark) {
    CSTDiagnoser::print_diagnostics(result.lex_diagnostics, chem::string_view(abs_path), "Lexer");
    CSTDiagnoser::print_diagnostics(result.parse_diagnostics, chem::string_view(abs_path), "Parser");
    if(benchmark) {
        if(result.lex_benchmark) {
            ASTProcessor::print_benchmarks(std::cout, "Lexer", result.lex_benchmark.get());
        }
        if(result.parse_benchmark) {
            ASTProcessor::print_benchmarks(std::cout, "Parser", result.parse_benchmark.get());
        }
    }
    std::cout << std::flush;
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

void check_imports_for_cycles(void* data_ptr, std::vector<ASTFileResult*>& files, ImportCycleHandler handler) {
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

void check_imports_for_cycles(ImportCycleCheckResult& out, std::vector<ASTFileResult*>& module_files) {
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

std::vector<ASTFileResult*> flatten(std::vector<ASTFileResult*>& files) {
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

inline bool is_node_exported(ASTNode* node) {
    switch(node->kind()) {
        case ASTNodeKind::IfStmt: {
            // top level if statements are retained at the moment
            return true;
        }
        default: {
            return node->specifier() == AccessSpecifier::Public;
        }
    }
}

int LabBuildCompiler::process_module_tcc(
        LabModule* mod,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        const std::string& mod_timestamp_file,
        const std::string& out_c_file,
        bool do_compile,
        std::stringstream& output_ptr,
        LabJobCBI* cbiJob
) {

    // variables
    const auto caching = options->is_caching_enabled;
    const auto verbose = options->verbose;
    const bool is_use_obj_format = options->use_mod_obj_format;

    auto& resolver = *processor.resolver;

    // these files are only the direct files present in the module
    // for example every file (nested included) in a directory
    // however it won't include files imported from another module
    std::vector<ASTFileMetaData> direct_files;

    // this would determine the direct files for the module
    processor.determine_mod_files(direct_files, mod);

    // let's check object file if it already exists
    if(fs::exists(mod->object_path.to_view())) {

        if (verbose) {
            std::cout << "[lab] " << "found object file '" << mod->object_path << "', checking timestamp." << std::endl;
        }

        // let's check if module timestamp file exists and is valid (files haven't changed)
        if (compare_mod_timestamp(direct_files, mod_timestamp_file)) {

            if (verbose) {
                std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "'" << std::endl;
                std::cout << "[lab] " << "reusing found object file" << std::endl;
            }

            // exe->linkables.emplace_back(mod->object_path.copy());
            return 0;

        } else if (verbose) {
            std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;
        }

    } else if(verbose) {
        std::cout << "[lab] " << "couldn't find object file at '" << mod->object_path << "'" << std::endl;
    }

    // ---------- HEAVY WORK BEGINS HERE ----------------
    // -------- (Tyla "omg, this is so heavy") ----------
    // ------- we must use any cache, before this --------

    // this will include the direct files (nested included) for the module
    // these will be lexed and parsed
    // these direct files will have "imports" field, which can be accessed to check each file's imports
    std::vector<ASTFileResult*> module_files;

    // this would import these direct files (lex and parse), into the module files
    // the module files will have imports, any file imported (from this module or external module will be included)
    if(!processor.import_mod_files(pool, module_files, direct_files, mod)) {
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

    // preparing translation
    c_visitor.prepare_translate();

    if(verbose) {
        std::cout << "[lab] " << "detecting import cycles in the imports" << std::endl;
    }

    // check module files for import cycles (direct or indirect)
    ImportCycleCheckResult importCycle { false, loc_man };
    check_imports_for_cycles(importCycle, module_files);
    if(importCycle.has_cyclic_dependencies) {
        return 1;
    }

    if(verbose) {
        std::cout << "[lab] " << "flattening the module import graph" << std::endl;
    }

    // get the files flattened
    auto flattened_files = flatten(module_files);

    if(verbose) {
        std::cout << "[lab] " << "resolving symbols in the module" << std::endl;
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = processor.sym_res_module(flattened_files);
    if(sym_res_status == 1) {
        return 1;
    }

    if(verbose) {
        std::cout << "[lab] " << "compiling module files" << std::endl;
    }

    // compile the whole module
    processor.translate_module(
            c_visitor, mod, flattened_files
    );

    if(verbose) {
        std::cout << "[lab] " << "disposing symbols in the module" << std::endl;
    }

    // going over each file in the module, to remove non-public nodes
    // so when we declare the nodes in other module, we don't consider non-public nodes
    // because non-public nodes are only present in the module allocator which will be cleared
    for(const auto file : module_files) {
        auto file_unit = processor.compiled_units.find(file->abs_path);
        if(file_unit != processor.compiled_units.end()) {
            auto& nodes = file_unit->second.scope.body.nodes;
            auto itr = nodes.begin();
            while(itr != nodes.end()) {
                auto& node = *itr;
                if(is_node_exported(node)) {
                    itr++;
                } else {
                    itr = nodes.erase(itr);
                }
            }
        }
    }

    // dispose module symbols in symbol resolver
    // resolver.dispose_module_symbols_now(mod->name.data());

    // disposing data
    mod_allocator->clear();

    // getting the c program
    const auto& program = output_ptr.str();

    // compiling the c program, if required
    if(do_compile) {
        auto obj_path = mod->object_path.to_std_string();
        if(verbose) {
            std::cout << "[lab] emitting the module '" << mod->name <<  "' object file at path '" << obj_path << '\'' << std::endl;
        }
        const auto compile_c_result = compile_c_string(options->exe_path.data(), program.c_str(), obj_path, false, options->benchmark, options->outMode == OutputMode::DebugComplete);
        if (compile_c_result == 1) {
            const auto out_path = resolve_sibling(obj_path, mod->name.to_std_string() + ".debug.c");
            writeToFile(out_path, program);
            std::cerr << rang::fg::red << "[lab] couldn't build module '" << mod->name.data() << "' due to error in translation, translated C written at " << out_path << rang::fg::reset << std::endl;
            return 1;
        }
        // exe->linkables.emplace_back(obj_path);
        generated[mod] = obj_path;
        if(caching) {
            save_mod_timestamp(direct_files, mod_timestamp_file);
        }
    }

    // writing the translated c file (if user required)
    if(!out_c_file.empty()) {
        if(mod->type == LabModuleType::CFile) {
            copyFile(mod->paths[0].to_view(), out_c_file);
        } else {
            writeToFile(out_c_file, program);
        }
    }

    if(cbiJob && c_visitor.compiler_interfaces) {
        auto& compiler_interfaces = *c_visitor.compiler_interfaces;
        auto cbiName = cbiJob->name.to_std_string();
        auto& cbiData = binder.data[cbiName];
        auto bResult = binder.compile(
                cbiData,
                program,
                compiler_interfaces,
                processor
        );
        if(options->verbose || options->benchmark || !bResult.error.empty()) {
            for(auto& diag : binder.diagnostics) {
                std::cerr << rang::fg::red << "[lab:cbi] " << diag << std::endl << rang::fg::reset;
            }
        }
        binder.diagnostics.clear();
        if(!bResult.error.empty()) {
            auto out_path = resolve_rel_child_path_str(cbiJob->build_dir.data(),mod->name.to_std_string() + ".2c.c");
            writeToFile(out_path, program);
            std::cerr << rang::fg::red << "[lab] failed to compile CBI module with name '" << mod->name.data() << "' in '" << cbiJob->name.data() << "' with error '" << bResult.error << "' written at '" << out_path << '\'' << rang::fg::reset << std::endl;
            return 1;
        }
        compiler_interfaces.clear();
        // marking entry of the cbi module, if this module is the entry
        if(cbiJob->entry_module == mod) {
            cbiData.entry_module = bResult.module;
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

    // these files are only the direct files present in the module
    // for example every file (nested included) in a directory
    // however it won't include files imported from another module
    std::vector<ASTFileMetaData> direct_files;

    // this would determine the direct files for the module
    processor.determine_mod_files(direct_files, mod);

    // let's check object file if it already exists
    if(fs::exists(mod->object_path.to_view())) {

        if (verbose) {
            std::cout << "[lab] " << "found object file '" << mod->object_path << "', checking timestamp." << std::endl;
        }

        // let's check if module timestamp file exists and is valid (files haven't changed)
        if (compare_mod_timestamp(direct_files, mod_timestamp_file)) {

            if (verbose) {
                std::cout << "[lab] " << "found valid module timestamp file at '" << mod_timestamp_file << "'" << std::endl;
                std::cout << "[lab] " << "reusing found object file" << std::endl;
            }

            // exe->linkables.emplace_back(mod->object_path.copy());
            return 0;

        } else if (verbose) {
            std::cout << "[lab] " << "couldn't find module timestamp file at '" << mod_timestamp_file << "' or it's not valid since files have changed" << std::endl;
        }

    } else if(verbose) {
        std::cout << "[lab] " << "couldn't find object file at '" << mod->object_path << "'" << std::endl;
    }

    // ---------- HEAVY WORK BEGINS HERE ----------------
    // -------- (Tyla "omg, this is so heavy") ----------
    // ------- we must use any cache, before this --------

    // this will include the direct files (nested included) for the module
    // these will be lexed and parsed
    // these direct files will have "imports" field, which can be accessed to check each file's imports
    std::vector<ASTFileResult*> module_files;

    // this would import these direct files (lex and parse), into the module files
    // the module files will have imports, any file imported (from this module or external module will be included)
    processor.import_mod_files(pool, module_files, direct_files, mod);

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
        std::cout << "[lab] " << "detecting import cycles in the imports" << std::endl;
    }

    // check module files for import cycles (direct or indirect)
    ImportCycleCheckResult importCycle { false, loc_man };
    check_imports_for_cycles(importCycle, module_files);
    if(importCycle.has_cyclic_dependencies) {
        return 1;
    }

    if(verbose) {
        std::cout << "[lab] " << "flattening the module import graph" << std::endl;
    }

    // get the files flattened
    auto flattened_files = flatten(module_files);

    if(verbose) {
        std::cout << "[lab] " << "resolving symbols in the module" << std::endl;
    }

    // symbol resolve all the files in the module
    const auto sym_res_status = processor.sym_res_module(flattened_files);
    if(sym_res_status == 1) {
        return 1;
    }

    if(verbose) {
        std::cout << "[lab] " << "compiling module files" << std::endl;
    }

    // compile the whole module
    processor.compile_module(
            gen, mod, flattened_files
    );

    if(verbose) {
        std::cout << "[lab] " << "disposing symbols in the module" << std::endl;
    }

    // going over each file in the module, to remove non-public nodes
    // so when we declare the nodes in other module, we don't consider non-public nodes
    // because non-public nodes are only present in the module allocator which will be cleared
    for(const auto file : module_files) {
        auto file_unit = processor.compiled_units.find(file->abs_path);
        if(file_unit != processor.compiled_units.end()) {
            auto& nodes = file_unit->second.scope.body.nodes;
            auto itr = nodes.begin();
            while(itr != nodes.end()) {
                auto& node = *itr;
                if(is_node_exported(node)) {
                    itr++;
                } else {
                    itr = nodes.erase(itr);
                }
            }
        }
    }

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
            // exe->linkables.emplace_back(gen_path);
            generated[mod] = gen_path;
            if(caching) {
                save_mod_timestamp(direct_files, mod_timestamp_file);
            }
        }
    } else {
        std::cerr << "[lab] failed to emit file " << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data()) << " " << std::endl;
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

int compile_c_or_cpp_module(LabBuildCompiler* compiler, LabModule* mod) {
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
    compiler->generated[mod] = mod->object_path.to_std_string();
#else
    if(mod->type == LabModuleType::CPPFile) {
        std::cerr << rang::fg::yellow << "[lab] skipping compilation of C++ file '" << mod->paths[0] << '\'' << rang::fg::reset << std::endl;
        return 1;
    }
    const auto compile_result = compile_c_file(compiler->options->exe_path.data(), mod->paths[0].data(), mod->object_path.to_std_string(), false, false, false);
    if (compile_result == 1) {
        return 1;
    }
    compiler->generated[mod] = mod->object_path.to_std_string();
#endif
    return 0;
}

std::string create_mod_dir_get_timestamp_path(LabBuildCompiler* compiler, LabJobType job_type, const std::string& build_dir, LabModule* mod) {
    const auto verbose = compiler->options->verbose;
    const auto is_use_obj_format = compiler->options->use_mod_obj_format;
    // creating the module directory
    auto module_dir_path = resolve_rel_child_path_str(build_dir, mod->name.to_std_string());
    auto mod_obj_path = resolve_rel_child_path_str(module_dir_path,  (is_use_obj_format ? "object.o" : "object.bc"));
    auto mod_timestamp_file = resolve_rel_child_path_str(module_dir_path, "timestamp.dat");
    if(!module_dir_path.empty() && job_type != LabJobType::ToCTranslation) {
        const auto mod_dir_exists = fs::exists(module_dir_path);
        if (!mod_dir_exists) {
            if(verbose) {
                std::cout << "[lab] " << "creating module directory at path '" << module_dir_path << "'" << std::endl;
            }
            fs::create_directory(module_dir_path);
        }
    }
    if(job_type == LabJobType::Executable || job_type == LabJobType::ProcessingOnly || job_type == LabJobType::Library) {
        if (is_use_obj_format || mod->type == LabModuleType::CFile) {
            if (mod->object_path.empty()) {
                mod->object_path.append(mod_obj_path);
            }
        } else {
            if (mod->bitcode_path.empty()) {
                mod->bitcode_path.append(mod_obj_path);
            }
        }
    }
    return mod_timestamp_file;
}

inline std::string create_mod_dir_get_timestamp_path(LabBuildCompiler* compiler, LabJob* job, LabModule* mod) {
    return create_mod_dir_get_timestamp_path(compiler, job->type, job->build_dir.to_std_string(), mod);
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
    GlobalInterpretScope global(options->target_triple, nullptr, this, *job_allocator, loc_man);

    // a new symbol resolver for every executable
    SymbolResolver resolver(global, path_handler, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // TODO this is only required in CBI
    std::vector<std::string> compiler_interfaces;

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(global, mangler, &output_ptr, *file_allocator, loc_man, job_type == LabJobType::CBI ? &compiler_interfaces : nullptr);
    ToCBackendContext c_context(&c_visitor);
    global.backend_context = (BackendContext*) &c_context;

    // the processor we use
    ASTProcessor processor(path_handler, options, mod_storage, loc_man, &resolver, binder, *job_allocator, *mod_allocator, *file_allocator);

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

    // allocating required variables before going into loop
    bool do_compile = job_type != LabJobType::ToCTranslation && job_type != LabJobType::CBI;

    // create cbi before hand, for reserving allocation
    if(job_type == LabJobType::CBI) {
        if(verbose) {
            std::cout << "[lab] " << "creating a compiler binding interface for '" << exe->name << "'" << std::endl;
        }
        binder.create_cbi(exe->name.to_std_string(), dependencies.size());
    }

    // compile dependent modules for this executable
    for(auto mod : dependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing module '" << mod->name << '\'' << std::endl;
        }

        // check we haven't already generated this module
        auto found = generated.find(mod);
        if(found != generated.end() && job_type != LabJobType::ToCTranslation) {
            exe->linkables.emplace_back(found->second);
            continue;
        }

        // creating the module directory and getting the timestamp file path
        const auto mod_timestamp_file = create_mod_dir_get_timestamp_path(this, job, mod);

        if(do_compile) {
            switch (mod->type) {
                case LabModuleType::CPPFile: {
                    // TODO we cannot yet compile cpp module when linking using tcc
                    // that's because we must figure out if this is a compiler executable and use clang
                    // to emit a executable that tiny cc can link easily, however this might not be possible
                    // since clang may not generate code like tiny cc does
                    std::cerr << rang::fg::yellow << "[lab] skipping compilation of C++ file '" << mod->paths[0] << '\'' << rang::fg::reset << std::endl;
                    continue;
                }
                case LabModuleType::CFile: {
                    const auto c_res = compile_c_or_cpp_module(this, mod);
                    if(c_res == 0) {
                        job->linkables.emplace_back(mod->object_path.copy());
                        continue;
                    } else {
                        return 1;
                    }
                }
                case LabModuleType::ObjFile:
                    exe->linkables.emplace_back(mod->paths[0].copy());
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
                    out_c_file = resolve_rel_child_path_str(job->build_dir.data(),mod->name.to_std_string() + ".2c.c");
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

        // get the cbi job pointer, if it's a cbi
        LabJobCBI* cbiJob = get_job_type == LabJobType::CBI ? (LabJobCBI*) job : nullptr;

        const auto result = process_module_tcc(mod, processor, c_visitor, mod_timestamp_file, out_c_file, do_compile, output_ptr, cbiJob);
        if(result == 1) {
            return 1;
        }

        if(do_compile) {
            job->linkables.emplace_back(mod->object_path.copy());
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
    GlobalInterpretScope global(options->target_triple, nullptr, this, *job_allocator, loc_man);

    // a new symbol resolver for every executable
    SymbolResolver resolver(global, path_handler, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    auto& job_alloc = *job_allocator;
    // a single c translator across this entire job
    CTranslator cTranslator(job_alloc, options->is64Bit);
    ASTProcessor processor(path_handler, options, mod_storage, loc_man, &resolver, binder, job_alloc, *mod_allocator, *file_allocator);
    CodegenOptions code_gen_options;
    if(cmd) {
        code_gen_options.fno_unwind_tables = cmd->has_value("", "fno-unwind-tables");
        code_gen_options.fno_asynchronous_unwind_tables = cmd->has_value("", "fno-asynchronous-unwind-tables");
        code_gen_options.no_pie = cmd->has_value("no-pie", "no-pie");
    }
    Codegen gen(code_gen_options, global, mangler, options->target_triple, options->exe_path, options->is64Bit, *file_allocator);
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

    // compile dependent modules for this executable
    for(auto mod : dependencies) {

        if(verbose) {
            std::cout << "[lab] " << "processing module '" << mod->name << '\'' << std::endl;
        }

        // check we haven't already generated this module
        auto found = generated.find(mod);
        if(found != generated.end()) {
            job->linkables.emplace_back(found->second);
            continue;
        }

        // creating the module directory and getting the timestamp file path
        const auto mod_timestamp_file = create_mod_dir_get_timestamp_path(this, job, mod);

        // handle c and cpp file modules
        switch (mod->type) {
            case LabModuleType::CFile:
            case LabModuleType::CPPFile: {
                const auto c_res = compile_c_or_cpp_module(this, mod);
                if(c_res == 0) {
                    job->linkables.emplace_back(mod->object_path.copy());
                    continue;
                } else {
                    return 1;
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
    const std::string_view& target_triple
) {
    int link_result;
#ifdef COMPILER_BUILD
    std::vector<std::string> data;
    for(auto& obj : linkables) {
        data.emplace_back(obj.data());
    }
    link_result = link_objects(data, output_path, comp_exe_path, flags, target_triple);
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

int LabBuildCompiler::link(std::vector<chem::string>& linkables, const std::string& output_path) {
    std::vector<std::string> flags;
#ifdef COMPILER_BUILD
    if(options->no_pie) {
        flags.emplace_back("-no-pie");
    }
#endif
    if(is_debug(options->outMode)) {
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
    return link_objects(options->exe_path, linkables, output_path, flags, options->target_triple);
}

int LabBuildCompiler::do_executable_job(LabJob* job) {
    auto result = process_modules(job);
    if(result == 1) {
        return 1;
    }
    // link will automatically detect the extension at the end
    return link(job->linkables, job->abs_path.to_std_string());
}

int LabBuildCompiler::do_library_job(LabJob* job) {
    auto result = process_modules(job);
    if(result == 1) {
        return 1;
    }
    // link will automatically detect the extension at the end
    return link(job->linkables, job->abs_path.to_std_string());
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
        CTranslator cTranslator(*mod_allocator, options->is64Bit);
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
    std::cerr << "Translating c files to chemical can only be done by the compiler executable, please check using compiler::is_clang_based() in build.lab" << std::endl;
    return 1;
#endif
}

LabModule* LabBuildCompiler::create_module_for_dependency(
        LabBuildContext& context,
        BuildLabModuleDependency& dependency,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        std::stringstream& output_ptr
) {

    auto& module_path = dependency.module_dir_path;
    const auto buildLabPath = resolve_rel_child_path_str(module_path, "build.lab");
    if(std::filesystem::exists(buildLabPath)) {

        // build lab file into a tcc state
        // TODO verify the build method signature in the build.lab file
        const auto state = built_lab_file(
            context, buildLabPath, processor, c_visitor, output_ptr
        );

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

        // since this file belongs to this module, we should set it (since initially this file gets assigned the module it's import is in)
        if(dependency.importer_file) {
            dependency.importer_file->module = &modPtr->module_scope;
        }

        return modPtr;

    } else {

        const auto modFilePath = resolve_rel_child_path_str(module_path, "chemical.mod");
        if(std::filesystem::exists(modFilePath)) {

            return build_module_from_mod_file(context, dependency, modFilePath, processor, c_visitor, output_ptr);

        } else {

            std::cerr << rang::fg::red << "error:" << rang::fg::reset << " module '" << dependency.scope_name << ':' << dependency.mod_name << '\'' << " at path '" << dependency.module_dir_path << "' doesn't contain a 'build.lab' or 'chemical.mod' therefore cannot be imported" << std::endl;
            return nullptr;

        }


    };

}

LabModule* LabBuildCompiler::build_module_from_mod_file(
        LabBuildContext& context,
        BuildLabModuleDependency& dependency,
        const std::string& modFilePath,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        std::stringstream& output_ptr
) {

    auto& lab_processor = processor;
    auto& lab_resolver = *processor.resolver;
    auto& module_dir = dependency.module_dir_path;
    const auto verbose = options->verbose;

    auto buildLabFileId = loc_man.encodeFile(modFilePath);
    ASTFileResult modResult(buildLabFileId, modFilePath, (ModuleScope*) nullptr);

    if(verbose) {
        std::cout << "[lab] " << "building mod file '" << modFilePath << '\'' << std::endl;
    }

    // import the file into result (lex and parse)
    if(!lab_processor.import_chemical_mod_file(modResult, buildLabFileId, modFilePath)) {
        return nullptr;
    }

    // printing results for module file parsing
    print_results(modResult, modFilePath, true);

    // probably an error during parsing
    if(!modResult.continue_processing) {
        if(verbose) {
            std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't import the mod file at '" << modFilePath << "' due to errors" << std::endl;
        }
        return nullptr;
    }

    // let's create a module for this .mod file
    const auto module = new LabModule(LabModuleType::Directory, chem::string(""), chem::string(""));
    context.storage.insert_module_ptr_dangerous(module);

    // TODO: support allowing src directory inside the .mod file
    // since currently we don't support custom source directory
    // we'll assume the source is present in 'src' directory and and use that as module directory path
    auto srcDirPath = resolve_rel_child_path_str(module_dir, "src");
    module->paths.emplace_back(srcDirPath);

    // update the scope and module name if user wrote package declaration in the mod file
    auto& nodes = modResult.unit.scope.body.nodes;
    if(nodes[0]->kind() == ASTNodeKind::PackageDef) {
        const auto def = nodes[0]->as_package_def_unsafe();
        module->update_mod_name(chem::string(def->scope_name), chem::string(def->module_name));
    } else {
        // otherwise we should try to get it from the import dependency
        module->update_mod_name(chem::string(dependency.scope_name), chem::string(dependency.mod_name));
    }

    // lets update the module scope of the mod file ast result, which we set to nullptr initially
    modResult.unit.scope.set_parent(&module->module_scope);
    modResult.module = &module->module_scope;

    if(verbose) {
        std::cout << "[lab] " << "created module for '" << module->scope_name << ':' << module->name << "'" << std::endl;
    }

    // module dependencies we determined from directly imported files
    std::vector<BuildLabModuleDependency> buildLabModuleDependencies;

    // figuring out module dependencies from import statements within the .mod file

    // some variables for processing
    std::string imp_module_dir_path;
    chem::string_view imp_scope_name;
    chem::string_view imp_mod_name;
    for(const auto stmt : nodes) {
        if(stmt->kind() == ASTNodeKind::ImportStmt) {
            const auto impStmt = stmt->as_import_stmt_unsafe();
            imp_module_dir_path.clear();
            imp_scope_name = "";
            imp_mod_name = "";
            if(impStmt->filePath.empty()) {
                const auto idSize = impStmt->identifier.size();
                if (idSize == 1) {
                    imp_mod_name = impStmt->identifier[0];
                } else if (idSize == 2) {
                    imp_scope_name = impStmt->identifier[0];
                    imp_mod_name = impStmt->identifier[1];
                } else {
                    // TODO handle the error
                }
                auto result = path_handler.resolve_lib_dir_path(imp_scope_name, imp_mod_name);
                if(result.error.empty()) {
                    imp_module_dir_path.append(result.replaced);
                } else {
                    // TODO handle the error
                }
            } else {
                imp_module_dir_path.append(impStmt->filePath.data(), impStmt->filePath.size());
            }
            buildLabModuleDependencies.emplace_back(std::move(imp_module_dir_path), nullptr, imp_scope_name, imp_mod_name);
        }
    }

    // these are modules imported by the build.lab
    // however we must build their build.lab or chemical.mod into a LabModule*
    for(auto& mod_ptr : buildLabModuleDependencies) {
        // get the module pointer
        const auto modDependency = create_module_for_dependency(context, mod_ptr, lab_processor, c_visitor, output_ptr);
        if(modDependency == nullptr) {
            return nullptr;
        }
        module->dependencies.emplace_back(modDependency);
    }

    // clear the allocators
    mod_allocator->clear();
    file_allocator->clear();

    // return the state
    return module;

}

bool LabBuildCompiler::compile_dependencies_tcc(
        LabBuildContext& context,
        std::vector<BuildLabModuleDependency>& dependencies,
        std::vector<LabModule*>& outModDependencies,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        std::stringstream& output_ptr
) {

    // figure out path for lab modules directory
    const auto lab_mods_dir = resolve_rel_child_path_str(options->build_dir, "lab/modules");

    // direct module dependencies (in no valid order)
    std::vector<LabModule*> mod_dependencies;

    // these are modules imported by the build.lab
    // however we must build their build.lab or chemical.mod into a LabModule*
    for(auto& mod_ptr : dependencies) {
        // get the module pointer
        const auto mod = create_module_for_dependency(context, mod_ptr, processor, c_visitor, output_ptr);
        if(mod == nullptr) {
            return false;
        }
        mod_dependencies.emplace_back(mod);
    }

    // in the order of least dependence
    outModDependencies = flatten_dedupe_sorted(mod_dependencies);
    for(const auto mod : outModDependencies) {

        // the timestamp file is what determines whether the module needs to be rebuilt again
        const auto timestamp_path = create_mod_dir_get_timestamp_path(this, LabJobType::ProcessingOnly, lab_mods_dir, mod);

        // the c output for this module, so we can debug
        const auto out_c_path = resolve_sibling(timestamp_path, "mod.2c.c");

        // compile the module
        const auto module_result = process_module_tcc(mod, processor, c_visitor, timestamp_path, out_c_path, true, output_ptr, nullptr);
        if(module_result == 1) {
            return false;
        }

        // since the job was successful, we can expect an object file at module's object_path
        // we'll use this object file by linking it with tcc
    }

    return true;

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
        const std::string& path,
        ASTProcessor& processor,
        ToCAstVisitor& c_visitor,
        std::stringstream& output_ptr
) {

    auto& lab_processor = processor;
    auto& lab_resolver = *processor.resolver;
    const auto verbose = options->verbose;

    if(verbose) {
        std::cout << "[lab] building lab file at '" << path << '\'' << std::endl;
    }

    LabModule chemical_lab_module(LabModuleType::Files, chem::string("chemical"), chem::string("lab"));
    auto buildLabFileId = loc_man.encodeFile(path);
    ASTFileMetaData buildLabMetaData(buildLabFileId, &chemical_lab_module.module_scope, path, path, "");
    ASTFileResult labFileResult(buildLabFileId, path, &chemical_lab_module.module_scope);

    // import the file into result (lex and parse)
    lab_processor.parse_on_job_allocator = true;
    lab_processor.import_file(labFileResult, buildLabMetaData.file_id, buildLabMetaData.abs_path);
    lab_processor.parse_on_job_allocator = false;

    // printing results for module file parsing
    print_results(labFileResult, path, true);

    // probably an error during parsing
    if(!labFileResult.continue_processing) {
        if(verbose) {
            std::cout << "[lab] " << rang::fg::red << "error: " << rang::fg::reset << "couldn't import the 'build.lab' file at '" << path << "' due to errors" << std::endl;
        }
        return nullptr;
    }

    // figure out files imported by this file
    std::vector<ASTFileMetaData> imports;
    lab_processor.figure_out_direct_imports(buildLabMetaData, labFileResult.unit.scope.body.nodes, imports);

    // module dependencies we determined from directly imported files
    std::vector<BuildLabModuleDependency> buildLabModuleDependencies;

    // direct imports tell us which modules the build.lab depends upon
    // this modules would be put into context.storage
    for(auto& imported : imports) {
        lab_processor.figure_out_module_dependency_based_on_import(imported, buildLabMetaData, buildLabModuleDependencies);
    }

    // compile all dependencies to object files
    if(!compile_dependencies_tcc(context, buildLabModuleDependencies, chemical_lab_module.dependencies, lab_processor, c_visitor, output_ptr)) {
        return nullptr;
    }

    // if has imports, we import those files, this would just hit caches
    // but it's required to build a proper import tree
    if(!imports.empty()) {
        const auto success = lab_processor.import_chemical_files(pool, labFileResult.imports, imports);
        if(!success) {
            return nullptr;
        }
    }

    // preparing translation
    c_visitor.prepare_translate();

    {

        std::vector<ASTFileResult*> files_to_flatten = { &labFileResult };

        // check module files for import cycles (direct or indirect)
        ImportCycleCheckResult importCycle { false, loc_man };
        check_imports_for_cycles(importCycle, files_to_flatten);
        if(importCycle.has_cyclic_dependencies) {
            return nullptr;
        }

        auto module_files = flatten(files_to_flatten);

        // symbol resolve all the files in the module
        const auto sym_res_status = lab_processor.sym_res_module_drop(module_files);
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

                if (!verify_app_build_func_type(found, file.abs_path)) {
                    return nullptr;
                }

                // expose the last file's build method, so it's callable
                found->set_specifier_fast(AccessSpecifier::Public);

            } else if (file.abs_path.ends_with(".lab")) {

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
                    const auto new_scope = new (job_allocator->allocate_released<ModuleScope>()) ModuleScope("lab", chem::string_view(name_str, name_size));
                    file.module = new_scope;
                    file.unit.scope.set_parent(new_scope);

                }
            }

            i++;
        }

        // translating the build.lab module
        lab_processor.translate_module(
            c_visitor, &chemical_lab_module, module_files
        );

    }

    // compiling the c output from build.labs
    const auto& str = output_ptr.str();

    // creating a new tcc state
    const auto state = setup_tcc_state(options->exe_path.data(), "", true, options->outMode == OutputMode::DebugComplete);
    if(state == nullptr) {
        const auto out_path = resolve_rel_child_path_str(options->build_dir, "build.lab.c");
        writeToFile(out_path, str);
        std::cerr << rang::fg::red << "[lab] couldn't build lab file due to error in translation, translated C written at " << out_path << rang::fg::reset << std::endl;
        return nullptr;
    } else {
        // TODO place a check to only output this when need be
        const auto out_path = resolve_rel_child_path_str(options->build_dir, "build.lab.c");
        writeToFile(out_path, str);
    }

    // add module object files
    for(const auto dep : chemical_lab_module.dependencies) {
        if(tcc_add_file(state, dep->object_path.data()) == -1) {
            std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " failed to add module '" << dep->scope_name << ':' << dep->name <<  "' in compilation of 'build.lab'" << std::endl;
            tcc_delete(state);
            return nullptr;
        }
    }

    // compiling the program
    if(tcc_compile_string(state, str.data()) == -1) {
        std::cerr << "[lab] " << rang::fg::red <<  "error:" << rang::fg::reset << " couldn't compile 'build.lab'" << std::endl;
        tcc_delete(state);
        return nullptr;
    }

    // prepare for jit
    prepare_tcc_state_for_jit(state);

    // import all compiler interfaces the lab files import
    for(const auto& interface : *c_visitor.compiler_interfaces) {
        if(!binder.import_compiler_interface(interface, state)) {
            std::cerr << rang::fg::red << "[lab] failed to import compiler binding interface '" << interface << '\'' << rang::fg::reset << std::endl;
            tcc_delete(state);
            return nullptr;
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

TCCState* LabBuildCompiler::built_lab_file(LabBuildContext& context, const std::string& path) {

#ifdef DEBUG
    if(!context.storage.get_modules().empty()) {
        throw std::runtime_error("please clean the module storage before using it with another lab file");
    }
#endif

    // a global interpret scope required to evaluate compile time things
    GlobalInterpretScope global(options->target_triple, nullptr, this, *job_allocator, loc_man);

    // creating symbol resolver for build.lab files only
    SymbolResolver lab_resolver(global, path_handler, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // the processor that does everything for build.lab files only
    ASTProcessor lab_processor(
            path_handler,
            options,
            mod_storage,
            loc_man,
            &lab_resolver,
            binder,
            *job_allocator,
            *mod_allocator,
            *file_allocator
    );

    // creates or rebinds the global container
    create_or_rebind_container(this, global, lab_resolver);

    // compiler interfaces the lab files imports
    std::vector<std::string> compiler_interfaces;
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(global, mangler, &output_ptr, *file_allocator, loc_man, &compiler_interfaces);
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

    // call the function
    return built_lab_file(
        context, path, lab_processor, c_visitor, output_ptr
    );

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
    prepare(
            &_job_allocator,
            &_mod_allocator,
            &_file_allocator
    );

    // do the jobs
    return do_jobs(this, data);

}

int LabBuildCompiler::do_job_allocating(LabJob* job) {

    return do_allocating((void*) job, [](LabBuildCompiler* compiler, void* data) {

        const auto job = (LabJob*) data;
        compiler->current_job = job;
        return compiler->do_job(job);

    });

}

int LabBuildCompiler::build_lab_file(LabBuildContext& context, const std::string& path) {

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
    auto state = built_lab_file(context, path);
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

    // call the root build.lab build's function
    build(&context);

    int job_result = 0;

    // generating outputs (executables)
    for(auto& exe : context.executables) {

        current_job = exe.get();

        job_result = do_job(exe.get());
        if(job_result == 1) {
            std::cerr << rang::fg::red << "[lab] " << "error performing job '" << exe->name.data() << "', returned status code 1" << rang::fg::reset << std::endl;
        }

        // clearing all allocations done in all the allocators
        _job_allocator.clear();
        _mod_allocator.clear();
        _file_allocator.clear();

    }

    // running the on_finished lambda
    if(context.on_finished) {
        context.on_finished(context.on_finished_data);
    }

    return job_result;

}

LabBuildCompiler::~LabBuildCompiler() {
    GlobalInterpretScope::dispose_container(container);
}