// Copyright (c) Qinetik 2024.

#include "rang.hpp"
#include "LabBuildCompiler.h"
#include "ast/types/LinkedType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "utils/Benchmark.h"
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
#include "preprocess/ShrinkingVisitor.h"
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
        if(child_type->kind() == BaseTypeKind::Linked && ((LinkedType*) child_type)->type == "Module") {
            return true;
        }
    }
    std::cerr << "[BuildLab] lab file at " << abs_path << " is a library, so it's build method's signature should return a Module*" << std::endl;
    return false;
}

static bool verify_app_build_func_type(FunctionDeclaration* found, const std::string& abs_path) {
    if(found->returnType->kind() != BaseTypeKind::Void) {
        std::cerr << "[BuildLab] the root .lab file at " << abs_path << " provided to compiler should use a void return type in it's build method" << std::endl;
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

namespace fs = std::filesystem;

/**
 * save mod timestamp data (modified date and file size) in a file that can be read later and compared
 * to check if files have changed
 */
void save_mod_timestamp(const std::vector<ASTFileResult*>& files, const std::string_view& output_file) {
    std::ofstream ofs(output_file.data(), std::ios::binary);
    size_t num_files = files.size();
    ofs.write(reinterpret_cast<const char*>(&num_files), sizeof(num_files));
    for (const auto file : files) {
        auto& file_abs_path = file->abs_path;
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

bool compare_mod_timestamp(const std::vector<std::string_view>& files, const std::string_view& prev_timestamp_file) {
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
        fs::path file_path(file);
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

LabBuildCompiler::LabBuildCompiler(CompilerBinder& binder, LabBuildCompilerOptions *options) : binder(binder), options(options), pool((int) std::thread::hardware_concurrency()) {

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

bool empty_diags(ASTFileResultNew& result) {
    return result.lex_diagnostics.empty() && result.parse_diagnostics.empty() && !result.lex_benchmark && !result.parse_benchmark;
}

void print_results(ASTFileResultNew& result, const std::string& abs_path, bool benchmark) {
    CSTDiagnoser::print_diagnostics(result.lex_diagnostics, abs_path, "Lexer");
    CSTDiagnoser::print_diagnostics(result.parse_diagnostics, abs_path, "Parser");
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

void check_imports_for_cycles(void* data_ptr, std::vector<ASTFileResultNew*>& files, ImportCycleHandler handler) {
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

void check_imports_for_cycles(ImportCycleCheckResult& out, std::vector<ASTFileResultNew*>& module_files) {
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

void flatten(std::vector<ASTFileResultNew*>& flat_out, std::unordered_map<std::string_view, bool>& done_files, ASTFileResultNew* single_file) {
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

std::vector<ASTFileResultNew*> flatten(std::vector<ASTFileResultNew*>& files) {
    std::vector<ASTFileResultNew*> flat_out;
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

int LabBuildCompiler::process_modules(LabJob* exe) {

    const auto job_type = exe->type;

    // the flag that forces usage of tcc
    const bool use_tcc = options->use_tcc || job_type == LabJobType::ToCTranslation || job_type == LabJobType::CBI;

    const auto caching = options->is_caching_enabled;

    std::cout << rang::bg::blue << rang::fg::black << "[BuildLab]" << ' ';
    switch(job_type) {
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
    if(!exe->name.empty()) {
        std::cout << ' ' << '\'' << exe->name.data() << '\'';
    }
    if(!exe->abs_path.empty()) {
        std::cout << " at path '" << exe->abs_path.data() << '\'';
    }
    std::cout << std::endl << rang::bg::reset << rang::fg::reset;

    std::string exe_build_dir = exe->build_dir.to_std_string();
    if(!exe_build_dir.empty()) {
        // create the build directory for this executable
        if (!std::filesystem::exists(exe_build_dir)) {
            std::filesystem::create_directory(exe_build_dir);
        }
    }

    // an interpretation scope for interpreting compile time function calls
    GlobalInterpretScope global(options->target_triple, nullptr, this, *job_allocator, loc_man);

    // a new symbol resolver for every executable
    SymbolResolver resolver(global, options->is64Bit, *file_allocator, mod_allocator, job_allocator);

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // TODO this is only required in CBI
    std::vector<std::string> compiler_interfaces;

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(global, &output_ptr, *file_allocator, loc_man, job_type == LabJobType::CBI ? &compiler_interfaces : nullptr);
    ToCBackendContext c_context(&c_visitor);

#ifdef COMPILER_BUILD
    auto& job_alloc = *job_allocator;
    // a single c translator across this entire job
    CTranslator cTranslator(job_alloc, options->is64Bit);
    ASTProcessor processor(options, loc_man, &resolver, binder, &cTranslator, job_alloc, *mod_allocator, *file_allocator);
    Codegen gen(global, options->target_triple, options->exe_path, options->is64Bit, *file_allocator, "");
    LLVMBackendContext g_context(&gen);
    CodegenEmitterOptions emitter_options;
    // set the context so compile time calls are sent to it
    global.backend_context = use_tcc ? (BackendContext*) &c_context : (BackendContext*) &g_context;
#else
    ASTProcessor processor(options, loc_man, &resolver, binder, *job_allocator, *mod_allocator, *file_allocator);
    global.backend_context = (BackendContext*) &c_context;
#endif

    // import executable path aliases
    processor.path_handler.path_aliases = std::move(exe->path_aliases);

    if(container) {
        // bind global container that contains namespaces like std and compiler
        // reusing it, if we created it before
        global.rebind_container(resolver, container);
    } else {
        // allow user the compiler (namespace) functions in @comptime
        // we create the new global container here once
        container = global.create_container(resolver);
    }

    if(use_tcc) {
        // clear build.lab c output
        output_ptr.clear();
        output_ptr.str("");
    }
#ifdef COMPILER_BUILD
    else {
        // emitter options allow to configure type of build (debug or release)
        // configuring the emitter options
        configure_emitter_opts(options->def_mode, &emitter_options);
        if (options->def_lto_on) {
            emitter_options.lto = true;
        }
        if(options->debug_ir) {
            emitter_options.debug_ir = true;
        }
        if (options->def_assertions_on) {
            emitter_options.assertions_on = true;
        }
    }
#endif

    // configure output path
    const bool is_use_obj_format = options->use_mod_obj_format;

    // flatten the dependencies
    auto dependencies = flatten_dedupe_sorted(exe->dependencies);

    // allocating required variables before going into loop
    int i;
    int compile_result = 0;
    bool do_compile = job_type != LabJobType::ToCTranslation && job_type != LabJobType::CBI;

    // create cbi before hand, for reserving allocation
    if(job_type == LabJobType::CBI) {
        binder.create_cbi(exe->name.to_std_string(), dependencies.size());
    }

    // compile dependent modules for this executable
    int mod_index = -1;
    for(auto mod : dependencies) {
        mod_index++;

        auto found = generated.find(mod);
        if(found != generated.end() && job_type != LabJobType::ToCTranslation) {
            exe->linkables.emplace_back(found->second);
            continue;
        }

        auto module_dir_path = resolve_rel_child_path_str(exe_build_dir, mod->name.to_std_string());
        auto mod_obj_path = resolve_rel_child_path_str(module_dir_path,  (is_use_obj_format ? "object.o" : "object.bc"));
        auto mod_timestamp_file = resolve_rel_child_path_str(module_dir_path, "timestamp.dat");
        if(!module_dir_path.empty()) {
            const auto mod_dir_exists = fs::exists(module_dir_path);
            if (!mod_dir_exists) {
                fs::create_directory(module_dir_path);
            }
        }

#ifdef COMPILER_BUILD
        // let c translator know that a new module has begin
        // so it can re-declare imported c headers
        cTranslator.module_begin();
#endif

        if(job_type == LabJobType::Executable || job_type == LabJobType::Library) {
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

        if(do_compile) {
            switch (mod->type) {
                case LabModuleType::CFile:
                case LabModuleType::CPPFile: {
                    std::cout << rang::bg::gray << rang::fg::black << "[BuildLab]";
                    if(mod->type == LabModuleType::CFile) {
                        std::cout << " Compiling c ";
                    } else {
                        std::cout << " Compiling c++ ";
                    }
                    if (!mod->name.empty()) {
                        std::cout << '\'' << mod->name.data() << "' ";
                    }
                    std::cout << "at path '" << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data())
                              << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
#ifdef COMPILER_BUILD
                    compile_result = compile_c_file_to_object(mod->paths[0].data(), mod->object_path.data(), options->exe_path, {});
                    if (compile_result == 1) {
                        break;
                    }
                    exe->linkables.emplace_back(mod->object_path.copy());
                    generated[mod] = mod->object_path.to_std_string();
                    continue;
#else
                    if(mod->type == LabModuleType::CPPFile) {
                        std::cerr << rang::fg::yellow << "[Tcc] skipping compilation of C++ file '" << mod->paths[0] << '\'' << rang::fg::reset << std::endl;
                        continue;
                    }
                    compile_result = compile_c_file(options->exe_path.data(), mod->paths[0].data(),
                                                    mod->object_path.to_std_string(), false, false, false);
                    if (compile_result == 1) {
                        break;
                    }
                    exe->linkables.emplace_back(mod->object_path.copy());
                    generated[mod] = mod->object_path.to_std_string();
                    continue;
#endif
                }
                case LabModuleType::ObjFile:
                    exe->linkables.emplace_back(mod->paths[0].copy());
                    continue;
                default:
                    break;
            }
            if (compile_result == 1) {
                break;
            }
        } else if (mod->type == LabModuleType::ObjFile
#ifdef COMPILER_BUILD
|| mod->type == LabModuleType::CFile
#endif
        ) {
            continue;
        }

        const auto mod_data_path = is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data();
        if(mod_data_path && do_compile) {
            std::cout << rang::bg::gray << rang::fg::black << "[BuildLab]" << " Building module ";
            if (!mod->name.empty()) {
                std::cout << '\'' << mod->name.data() << "' ";
            }
            std::cout << "at path '" << mod_data_path << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
        }

        std::vector<ASTFileResultNew*> module_files;
        processor.determine_mod_imports(pool, module_files, mod);

        if(use_tcc) {
            // preparing translation
            c_visitor.prepare_translate();
        }
#ifdef COMPILER_BUILD
        else {
            // prepare for code generation of this module
            gen.module_init(mod->name.to_std_string());
        }
#endif

        // start a module scope in symbol resolver, that we can dispose later
        resolver.module_scope_start();

        // importing files user imported using includes
        if(!mod->includes.empty()) {
            for(auto& include : mod->includes) {
                const auto abs_path = include.to_std_string();
                unsigned fileId = loc_man.encodeFile(abs_path);
                ASTFileResultNew imported_file;
                processor.import_chemical_file(imported_file, fileId, abs_path);
                auto& scope = imported_file.unit.scope;
                auto& nodes = scope.nodes;
                resolver.resolve_file(scope, abs_path);
#ifdef COMPILER_BUILD
                processor.compile_nodes(gen, nodes, abs_path);
#else
                processor.translate_to_c(c_visitor, nodes, abs_path);
#endif
            }
        }

#ifdef COMPILER_BUILD
        // importing c headers of the module before processing files
        if(!mod->headers.empty()) {
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
#endif

        // check module files for import cycles (direct or indirect)
        ImportCycleCheckResult importCycle { false, loc_man };
        check_imports_for_cycles(importCycle, module_files);
        if(importCycle.has_cyclic_dependencies) {
            compile_result = 1;
            break;
        }

        // get the files flattened
        auto flattened_files = flatten(module_files);

        // sequentially symbol resolve all the files in the module
        for(auto file_ptr : flattened_files) {

            auto& file = *file_ptr;

            auto imported = processor.shrinked_unit.find(file.abs_path);
            bool already_imported = imported != processor.shrinked_unit.end();

            // symbol resolution
            if(!already_imported) {
                processor.sym_res_file(file.unit.scope, file.is_c_file, file.abs_path);
                if (resolver.has_errors && !options->ignore_errors) {
                    std::cerr << rang::fg::red << "couldn't perform job due to errors during symbol resolution" << rang::fg::reset << std::endl;
                    compile_result = 1;
                    break;
                }
                resolver.reset_errors();
            }

            // clear everything allocated during symbol resolution of current file
            file_allocator->clear();

        }


        // sequentially compile each file
        i = 0;
        for(auto file_ptr : flattened_files) {

            auto& file = *file_ptr;
            auto& result = file;

            // check file exists
            if(file.abs_path.empty()) {
                std::cerr << rang::fg::red << "error: file not found '" << file.import_path << "'" << rang::fg::reset << std::endl;
                compile_result = 1;
                break;
            }

            if(!result.read_error.empty()) {
                std::cerr << rang::fg::red << "error: when reading file '" << file.abs_path << "' with message '" << result.read_error << "'" << rang::fg::reset << std::endl;
                compile_result = 1;
                break;
            }

            auto imported = processor.shrinked_unit.find(file.abs_path);
            bool already_imported = imported != processor.shrinked_unit.end();
            // already imported
            if(already_imported) {
                result.continue_processing = true;
                result.is_c_file = false;
            } else {
                // get the processed result
//                result = std::move(file);
            }

            ASTUnit& unit = already_imported ? imported->second : file.unit;

            // print the benchmark or verbose output received from processing
            if((options->benchmark || options->verbose) && !empty_diags(result)) {
                std::cout << rang::style::bold << rang::fg::magenta << "[Processing] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
                if(!already_imported) {
                    print_results(result, file.abs_path, options->benchmark);
                }
            }

            // do not continue processing
            if(!result.continue_processing) {
                std::cerr << rang::fg::red << "couldn't perform job due to errors during lexing or parsing file '" << file.abs_path << '\'' << rang::fg::reset << std::endl;
                compile_result = 1;
                break;
            }

            if(use_tcc) {
                // reset the c visitor to use with another file
                c_visitor.reset();
                if(already_imported) {
                    auto declared_in = unit.declared_in.find(mod);
                    if(declared_in == unit.declared_in.end()) {
                        // this is probably a different module, so we'll declare the file (if not declared)
                        processor.declare_in_c(c_visitor, unit.scope, file.abs_path);
                        unit.declared_in[mod] = true;
                    }
                } else {
                    // translating to c
                    processor.translate_to_c(c_visitor, unit.scope.nodes, file.abs_path);
                }
            }
#ifdef COMPILER_BUILD
            else {
                if(already_imported) {
                    auto declared_in = unit.declared_in.find(mod);
                    if(declared_in == unit.declared_in.end()) {
                        // this is probably a different module, so we'll declare the file (if not declared)
                        processor.declare_nodes(gen, unit.scope, file.abs_path);
                        unit.declared_in[mod] = true;
                    }
                } else {
                    // compiling the nodes
                    processor.compile_nodes(gen, unit.scope.nodes, file.abs_path);
                }
            }
#endif

            if(!already_imported) {
                if(options->verbose) {
                    std::cout << rang::fg::magenta << "[Shrinking] " << file.abs_path << rang::fg::reset << std::endl;
                }
                processor.shrink_nodes(shrinker, std::move(result.unit), file.abs_path);
            }

            // clear everything we allocated using file allocator to make it re-usable
            file_allocator->clear();

        }

        // going over each file in the module, to remove non-public nodes
        // so when we declare the nodes in other module, we don't consider non-public nodes
        // because non-public nodes are only present in the module allocator which will be cleared
        for(const auto file : module_files) {
            auto file_unit = processor.shrinked_unit.find(file->abs_path);
            if(file_unit != processor.shrinked_unit.end()) {
                auto& nodes = file_unit->second.scope.nodes;
                auto itr = nodes.begin();
                while(itr != nodes.end()) {
                    auto& node = *itr;
                    if(node->specifier() != AccessSpecifier::Public) {
                        itr = nodes.erase(itr);
                    } else {
                        itr++;
                    }
                }
            }
        }

        // no need to dispose symbols for last module
        if(mod_index < dependencies.size() - 1) {
            // dispose module symbols in symbol resolver
            resolver.dispose_module_symbols_now(mod->name.data());
        }

#ifdef COMPILER_BUILD
        if(!use_tcc && gen.has_errors) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during code generation" << rang::fg::reset << std::endl;
            compile_result = 1;
            break;
        }
#endif
        if(compile_result == 1) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during code generation" << rang::fg::reset << std::endl;
            break;
        }

#ifdef COMPILER_BUILD
        // finalizing the di builder
        gen.di.finalize();
#endif
        // disposing data
        mod_allocator->clear();

        if(use_tcc) {

            // getting the c program
            const auto& program = output_ptr.str();

            // compiling the c program, if required
            if(do_compile) {
                auto obj_path = mod->object_path.to_std_string();
                compile_result = compile_c_string(options->exe_path.data(), program.c_str(), obj_path, false, options->benchmark, options->def_mode == OutputMode::DebugComplete);
                if (compile_result == 1) {
                    const auto out_path = resolve_sibling(obj_path, mod->name.to_std_string() + ".debug.c");
                    writeToFile(out_path, program);
                    std::cerr << rang::fg::red << "[LabBuild] couldn't build module '" << mod->name.data() << "' due to error in translation, translated C written at " << out_path << rang::fg::reset << std::endl;
                    break;
                }
                exe->linkables.emplace_back(obj_path);
                generated[mod] = obj_path;
                if(caching) {
                    save_mod_timestamp(flattened_files, mod_timestamp_file);
                }
            }

            // writing the translated c file (if user required)
            if(job_type == LabJobType::ToCTranslation || !mod->out_c_path.empty()) {
                auto out_path = mod->out_c_path.to_std_string();
                if(out_path.empty()) {
                    if(!exe->build_dir.empty()) {
                        out_path = resolve_rel_child_path_str(exe->build_dir.data(),mod->name.to_std_string() + ".2c.c");
                    } else if(!exe->abs_path.empty()) {
                        out_path = exe->abs_path.to_std_string();
                    }
                }
                if(!out_path.empty()) {
                    if(mod->type == LabModuleType::CFile) {
                        copyFile(mod->paths[0].to_view(), out_path);
                    } else {
                        writeToFile(out_path, program);
                    }
                }
#ifdef DEBUG
                else {
                    throw std::runtime_error("couldn't figure out the output c path");
                }
#endif
            }

            if(job_type == LabJobType::CBI) {
                const auto cbiJob = (LabJobCBI*) exe;
                auto cbiName = exe->name.to_std_string();
                auto& cbiData = binder.data[cbiName];
                auto bResult = binder.compile(
                        cbiData,
                        program,
                        compiler_interfaces,
                        processor
                );
                if(options->verbose || options->benchmark || !bResult.error.empty()) {
                    for(auto& diag : binder.diagnostics) {
                        std::cerr << rang::fg::red << "[BuildLab:CBI] " << diag << std::endl << rang::fg::reset;
                    }
                }
                binder.diagnostics.clear();
                if(!bResult.error.empty()) {
                    auto out_path = resolve_rel_child_path_str(exe->build_dir.data(),mod->name.to_std_string() + ".2c.c");
                    writeToFile(out_path, program);
                    std::cerr << rang::fg::red << "[BuildLab] failed to compile CBI module with name '" << mod->name.data() << "' in '" << exe->name.data() << "' with error '" << bResult.error << "' written at '" << out_path << '\'' << rang::fg::reset << std::endl;
                    compile_result = 1;
                    break;
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

        }
#ifdef COMPILER_BUILD
        else {

            // which files to emit
            if(!mod->llvm_ir_path.empty()) {
                emitter_options.ir_path = mod->llvm_ir_path.data();
                if(options->debug_ir) {
                    gen.save_to_ll_file_for_debugging(mod->llvm_ir_path.data());
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

            // creating a object or bitcode file
            const bool save_result = gen.save_with_options(&emitter_options);
            if(save_result) {
                const auto gen_path = is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data();
                if(gen_path) {
                    exe->linkables.emplace_back(gen_path);
                    generated[mod] = gen_path;
                    if(caching) {
                        save_mod_timestamp(flattened_files, mod_timestamp_file);
                    }
                }
            } else {
                std::cerr << "[BuildLab] failed to emit file " << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data()) << " " << std::endl;
            }

        }
#endif

    }

    exe->path_aliases = std::move(processor.path_handler.path_aliases);

    return compile_result;


}

int link_objects(
    const std::string& comp_exe_path,
    std::vector<chem::string>& linkables,
    const std::string& output_path,
    const std::vector<std::string>& flags
) {
    int link_result;
#ifdef COMPILER_BUILD
    std::vector<std::string> data;
    for(auto& obj : linkables) {
        data.emplace_back(obj.data());
    }
    link_result = link_objects(data, output_path, comp_exe_path, flags);
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
    return link_objects(options->exe_path, linkables, output_path, flags);
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
        std::cerr << rang::fg::red << "[BuildLab] " << "couldn't open the file for writing translated chemical from c at '" << job->abs_path.data() << '\'' << rang::fg::reset << std::endl;
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

TCCState* LabBuildCompiler::built_lab_file(LabBuildContext& context, const std::string& path) {

    // set the build context
    build_context = &context;

    const auto lab_mem_size = 100000; // 100kb for the whole lab operations

    // the allocator is used in lab
    ASTAllocator lab_allocator(lab_mem_size);

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // a global interpret scope required to evaluate compile time things
    GlobalInterpretScope global(options->target_triple, nullptr, this, lab_allocator, loc_man);

    // creating symbol resolver for build.lab files only
    SymbolResolver lab_resolver(global, options->is64Bit, lab_allocator, &lab_allocator, &lab_allocator);

#ifdef COMPILER_BUILD
    // a single c translator is used to translate c files
    CTranslator cTranslator(lab_allocator, options->is64Bit);
#endif

    // the processor that does everything for build.lab files only
    ASTProcessor lab_processor(
            options,
            loc_man,
            &lab_resolver,
            binder,
#ifdef COMPILER_BUILD
            &cTranslator,
#endif
            lab_allocator,
            lab_allocator, // lab allocator is being used as a module level allocator
            lab_allocator
    );

    // get flat imports
    ASTFileResultNew blResult;
    auto buildLabFileId = loc_man.encodeFile(path);
    ASTFileMetaData buildLabMetaData(buildLabFileId, path, path);

    lab_processor.import_chemical_file(blResult, pool, buildLabMetaData);

    if(!blResult.continue_processing) {
        return nullptr;
    }

    int compile_result = 0;

    // compiler interfaces the lab files imports
    std::vector<std::string> compiler_interfaces;

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(global, &output_ptr, lab_allocator, loc_man, &compiler_interfaces);
    ToCBackendContext c_context(&c_visitor);

    // set the backend context
    global.backend_context = &c_context;

    // allow user the compiler (namespace) functions in @comptime
    if(container) {
        // reuse already created container
        global.rebind_container(lab_resolver, container);
    } else {
        // create a new container, once
        container = global.create_container(lab_resolver);
    }

    // preparing translation
    c_visitor.prepare_translate();

    // function can find the build method in lab file
    auto finder = [](SymbolResolver& resolver, const std::string& abs_path, bool error = true) -> FunctionDeclaration* {
        auto found = resolver.find("build");
        if(found) {
            auto func = found->as_function();
            if(func) {
                return func;
            } else if(error){
                std::cerr << "[LabBuild] expected build to be a function in the root lab build file " << abs_path << std::endl;
            }
        } else if(error) {
            std::cerr << "[LabBuild] No build method found in the root lab build file " << abs_path << std::endl;
        }
        return nullptr;
    };

    {

        std::vector<ASTFileResultNew*> files_to_flatten = { &blResult };

        // check module files for import cycles (direct or indirect)
        ImportCycleCheckResult importCycle { false, loc_man };
        check_imports_for_cycles(importCycle, files_to_flatten);
        if(importCycle.has_cyclic_dependencies) {
            return nullptr;
        }

        auto module_files = flatten(files_to_flatten);

        // symbol resolve all the files first
        for(const auto file_ptr : module_files) {

            auto& file = *file_ptr;

            // symbol resolution
            lab_processor.sym_res_file(file.unit.scope, file.is_c_file, file.abs_path);
            if (lab_resolver.has_errors && !options->ignore_errors) {
                compile_result = false;
                break;
            }
            lab_resolver.reset_errors();

        }

        // processing each build.lab file and creating C output
        int i = 0;
        for (const auto file_ptr : module_files) {

            auto& file = *file_ptr;

            auto& result = file;
            if (!result.continue_processing) {
                compile_result = false;
                break;
            }

            // print the benchmark or verbose output received from processing
            if((options->benchmark || options->verbose) && !empty_diags(result)) {
                std::cout << rang::style::bold << rang::fg::magenta << "[Processing] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
                print_results(result, file.abs_path, options->benchmark);
            }

            // the last build.lab file is whose build method is to be called
            bool is_last = i == module_files.size() - 1;
            if (is_last) {
                auto found = finder(lab_resolver, file.abs_path);
                if (!found) {
                    compile_result = 1;
                    break;
                }
                if (!verify_app_build_func_type(found, file.abs_path)) {
                    compile_result = 1;
                    break;
                }
                // expose the last file's build method, so it's callable
                found->set_specifier_fast(AccessSpecifier::Public);
            } else if (file.abs_path.ends_with(".lab")) {
                if (file.as_identifier.empty()) {
                    std::cerr
                            << "[BuildLab] lab file cannot be imported without an 'as' identifier in import statement, error importing "
                            << file.abs_path << std::endl;
                    compile_result = 1;
                    break;
                } else {
                    auto found = finder(lab_resolver, file.abs_path);
                    if (!found) {
                        compile_result = 1;
                        break;
                    }
                    if (!verify_lib_build_func_type(found, file.abs_path)) {
                        compile_result = 1;
                        break;
                    }
                    // put every imported file in it's own namespace so build methods don't clash
                    auto ns = new (lab_allocator.allocate<Namespace>()) Namespace(ZERO_LOC_ID(lab_allocator, file.as_identifier), nullptr, ZERO_LOC);
                    for (auto &node: result.unit.scope.nodes) {
                        node->set_parent(ns);
                    }
                    ns->nodes = std::move(result.unit.scope.nodes);
                    result.unit.scope.nodes.emplace_back(ns);
                }
            }

            // reset c visitor to use with another file
            c_visitor.reset();

            // translate build.lab file to c
            lab_processor.translate_to_c(c_visitor, result.unit.scope.nodes, file.abs_path);

            // shrinking the nodes
            lab_processor.shrink_nodes(shrinker, std::move(result.unit), file.abs_path);

            i++;
        }
    }

    // return if error occurred during processing of build.lab(s)
    if(compile_result == 1) {
        return nullptr;
    }

    // compiling the c output from build.labs
    const auto& str = output_ptr.str();
    auto state = compile_c_to_tcc_state(options->exe_path.data(), str.data(), "", true, options->def_mode == OutputMode::DebugComplete);

    if(state == nullptr) {
        const auto out_path = resolve_rel_child_path_str(context.build_dir, "build.lab.c");
        writeToFile(out_path, str);
        std::cerr << rang::fg::red << "[LabBuild] couldn't build lab file due to error in translation, translated C written at " << out_path << rang::fg::reset << std::endl;
        return nullptr;
    }

    // import all compiler interfaces the lab files import
    for(const auto& interface : compiler_interfaces) {
        if(!binder.import_compiler_interface(interface, state)) {
            std::cerr << rang::fg::red << "[LabBuild] failed to import compiler binding interface '" << interface << '\'' << rang::fg::reset << std::endl;
            tcc_delete(state);
            return nullptr;
        }
    }

    // relocate the code before calling
    tcc_relocate(state);

    // return the state
    return state;

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

    auto state = built_lab_file(context, path);
    if(!state) {
        return 1;
    }

    // automatic destroy
    TCCDeletor auto_del(state);

    // get the build method
    auto build = (void(*)(LabBuildContext*)) tcc_get_symbol(state, "build");
    if(!build) {
        std::cerr << rang::fg::red << "[LabBuild] couldn't get build function symbol in build.lab" << rang::fg::reset << std::endl;
        return 1;
    }

    // call the root build.lab build's function
    build(&context);

    // mkdir the build directory
    if(!std::filesystem::exists(context.build_dir)) {
        std::filesystem::create_directory(context.build_dir);
    }

    int job_result = 0;

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

    // generating outputs (executables)
    for(auto& exe : context.executables) {

        current_job = exe.get();

        job_result = do_job(exe.get());
        if(job_result == 1) {
            std::cerr << rang::fg::red << "[BuildLab]" << " error performing job '" << exe->name.data() << "', returned status code 1" << rang::fg::reset << std::endl;
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