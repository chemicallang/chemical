// Copyright (c) Qinetik 2024.

#include "rang.hpp"
#include "LabBuildCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "Utils.h"
#ifdef COMPILER_BUILD
#include "compiler/Codegen.h"
#endif
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include "compiler/ASTProcessor.h"
#include "ast/structures/Namespace.h"
#include "preprocess/ShrinkingVisitor.h"
#include "preprocess/2cASTVisitor.h"
#include "compiler/lab/LabBuildContext.h"
#include "integration/ide/bindings/BuildContextCBI.h"
#include "integration/libtcc/LibTccInteg.h"
#include "ctpl.h"
#include "compiler/InvokeUtils.h"
#include "utils/PathUtils.h"
#include "compiler/ASTCompiler.h"
#include "preprocess/RepresentationVisitor.h"
#include <sstream>
#include <filesystem>
#include <iostream>
#include <utility>
#include <functional>
#include "utils/FileUtils.h"

#ifdef COMPILER_BUILD
std::vector<std::unique_ptr<ASTNode>> TranslateC(
    const char *exe_path,
    const char *abs_path,
    const char *resources_path
);
#endif

static bool verify_lib_build_func_type(FunctionDeclaration* found, const std::string& abs_path) {
    if(found->returnType->kind() == BaseTypeKind::Pointer) {
        auto child_type = found->returnType->get_child_type();
        if(child_type->kind() == BaseTypeKind::Referenced && ((ReferencedType*) child_type.get())->type == "Module") {
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
    for(auto nested : file->dependencies) {
        recursive_dedupe(nested, imported, flat_map);
    }
    auto found = imported.find(file);
    if(found == imported.end()) {
        imported[file] = true;
        flat_map.emplace_back(file);
    }
}

/**
 * it creates a flat vector containing pointers to lab modules, sorted
 *
 * It de-dupes, meaning avoids duplicates, it won't add two pointers
 * that are same, so dependencies that occur again and again, would
 * only be compiled once
 *
 * why sort ? Modules that should be compiled first are present first
 * The first module that should be compiled is at zero index, The last
 * module would be the given module, compiled at last
 * TODO
 * 1 - avoid direct cyclic dependencies a depends on b and b depends on a
 * 2 - avoid indirect cyclic dependencies a depends on b and b depends on c and c depends on a
 */
std::vector<LabModule*> flatten_dedupe_sorted(LabModule* mod) {
    std::vector<LabModule*> modules;
    std::unordered_map<LabModule*, bool> imported;
    recursive_dedupe(mod, imported, modules);
    return modules;
}

/**
 * same as above, only it operates on multiple modules, it de-dupes the dependent modules
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

LabBuildCompiler::LabBuildCompiler(LabBuildCompilerOptions *options) : options(options), pool((int) std::thread::hardware_concurrency()) {

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
            return_int = do_to_c_job(job);
            break;
        case LabJobType::ToChemicalTranslation:
            return_int = do_to_chemical_job(job);
            break;
        case LabJobType::ProcessingOnly:
            return_int = process_modules(job);
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

int LabBuildCompiler::process_modules(LabJob* exe) {

    // the flag that forces usage of tcc
    const bool use_tcc = options->use_tcc;

    std::cout << rang::bg::blue << rang::fg::black << "[BuildLab]" << " Building ";
    if(exe->type == LabJobType::Executable) {
        std::cout << "executable";
    } else {
        std::cout << "library";
    }
    if(!exe->name.empty()) {
        std::cout << ' ' << '\'' << exe->name.data() << '\'';
    }
    std::cout << " at path '" << exe->abs_path.data() << '\'' << rang::bg::reset << rang::fg::reset << std::endl;

    std::string exe_build_dir;
    if(!exe->build_dir.empty()) {
        exe_build_dir = exe->build_dir.to_std_string();
        // create the build directory for this executable
        if (!std::filesystem::exists(exe_build_dir)) {
            std::filesystem::create_directory(exe_build_dir);
        }
    }

    // a new symbol resolver for every executable
    SymbolResolver resolver(options->is64Bit);

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(&output_ptr);

#ifdef COMPILER_BUILD
    ASTCompiler processor(options, &resolver);
    Codegen gen({}, options->target_triple, options->exe_path, options->is64Bit, "");
    CodegenEmitterOptions emitter_options;
#else
    ASTProcessor processor(options, &resolver);
#endif

    if(use_tcc) {
        // clear build.lab c output
        output_ptr.clear();
        output_ptr.str("");
        // allow user the compiler (namespace) functions in @comptime
        c_visitor.comptime_scope.prepare_compiler_namespace(resolver);
    }
#ifdef COMPILER_BUILD
    else {
        // creating codegen, processor  for this executable
        // prepare compiler functions in codegen scope
        gen.comptime_scope.prepare_compiler_namespace(resolver);
        // emitter options allow to configure type of build (debug or release)
        // configuring the emitter options
        configure_emitter_opts(options->def_mode, &emitter_options);
        if (options->def_lto_on) {
            emitter_options.lto = true;
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
    std::vector<FlatIGFile> flat_imports;
    int i;
    int compile_result = 0;

    // compile dependent modules for this executable
    for(auto mod : dependencies) {

        auto found = generated.find(mod);
        if(found != generated.end()) {
            exe->linkables.emplace_back(found->second);
            continue;
        }

        if(exe->type == LabJobType::Executable || exe->type == LabJobType::Library){
            auto obj_path = resolve_rel_child_path_str(exe_build_dir, mod->name.to_std_string() +
                                                                      (is_use_obj_format ? ".o" : ".bc"));
            if (is_use_obj_format || mod->type == LabModuleType::CFile) {
                if (mod->object_path.empty()) {
                    mod->object_path.append(obj_path);
                }
            } else {
                if (mod->bitcode_path.empty()) {
                    mod->bitcode_path.append(obj_path);
                }
            }
        }

        if(mod->type == LabModuleType::CFile) {
            std::cout << rang::bg::gray << rang::fg::black << "[BuildLab]" << " Compiling c ";
            if(!mod->name.empty()) {
                std::cout << '\'' << mod->name.data() << "' ";
            }
            std::cout << "at path '" << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data()) << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
        }

        switch(mod->type) {
            case LabModuleType::CFile: {
#ifdef COMPILER_BUILD
                compile_result = compile_c_file_to_object(mod->paths[0].data(), mod->object_path.data(), options->exe_path, {});
                if (compile_result == 1) {
                    break;
                }
                exe->linkables.emplace_back(mod->object_path.copy());
                generated[mod] = mod->object_path.to_std_string();
                continue;
#else
                compile_result = compile_c_file(options->exe_path.data(), mod->paths[0].data(), mod->object_path.to_std_string(), false, false, false);
                if(compile_result == 1) {
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
        }

        const auto mod_data_path = is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data();
        if(mod_data_path) {
            std::cout << rang::bg::gray << rang::fg::black << "[BuildLab]" << " Building module ";
            if (!mod->name.empty()) {
                std::cout << '\'' << mod->name.data() << "' ";
            }
            std::cout << "at path '" << mod_data_path << '\'' << rang::bg::reset << rang::fg::reset << std::endl;
        }

        // get flat file map of this module
        flat_imports = processor.determine_mod_imports(mod);

        // send all files for concurrent processing (lex and parse)
        std::vector<std::future<ASTImportResultExt>> futures;
        i = 0;
        for(const auto& file : flat_imports) {
            auto already_imported = processor.shrinked_nodes.find(file.abs_path);
            if(already_imported == processor.shrinked_nodes.end()) {
                futures.push_back(pool.push(concurrent_processor, i, file, &processor));
                i++;
            }
        }

#ifdef COMPILER_BUILD
        // prepare for code generation of this module
        gen.module_init(mod->name.to_std_string());
        gen.compile_begin();
#endif

        if(use_tcc) {
            // preparing translation
            c_visitor.prepare_translate();
        }

        ASTImportResultExt result { Scope { nullptr }, false, false, "" };

        // sequentially compile each file
        i = 0;
        for(const auto& file : flat_imports) {

            auto imported = processor.shrinked_nodes.find(file.abs_path);
            bool already_imported = imported != processor.shrinked_nodes.end();
            // already imported
            if(already_imported) {
                result.scope.nodes = std::move(imported->second);
                result.continue_processing = true;
                result.is_c_file = false;
            } else {
                // get the processed result
                result = std::move(futures[i].get());
                if(!result.continue_processing) {
                    compile_result = 1;
                    break;
                }
            }

            // print the benchmark or verbose output received from processing
            if((options->benchmark || options->verbose) && !result.cli_out.empty()) {
                std::cout << rang::style::bold << rang::fg::magenta << "[Processing] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
                std::cout << result.cli_out << std::flush;
            }

            // symbol resolution
            if(!already_imported) {
                processor.sym_res(result.scope, result.is_c_file, file.abs_path);
                if (resolver.has_errors) {
                    compile_result = 1;
                    break;
                }
                resolver.reset_errors();
            }

            if(use_tcc) {
                // reset the c visitor to use with another file
                c_visitor.reset();
                // translating to c
                processor.translate_to_c(c_visitor, result.scope, file);
            }
#ifdef COMPILER_BUILD
            else {
                // compiling the nodes
                processor.compile_nodes(&gen, result.scope, file);
            }
#endif

            if(!already_imported) {
                processor.shrink_nodes(shrinker, result.scope, file);
                i++;
            }

        }

        futures.clear();
        processor.end();

        if(use_tcc) {
            auto obj_path = mod->object_path.to_std_string();
            compile_result = compile_c_string(options->exe_path.data(), output_ptr.str().data(), obj_path, false, options->benchmark, is_debug(options->def_mode));
            if(compile_result == 1) {
                break;
            }
            exe->linkables.emplace_back(obj_path);
            generated[mod] = obj_path;
            // clear the current c string
            output_ptr.clear();
            output_ptr.str("");
        }

#ifdef COMPILER_BUILD
        if(!use_tcc) {
            gen.compile_end();
        }
        // errors in a single module means no linking
        if (gen.has_errors) {
            compile_result = 1;
            break;
        }
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
            }
        } else {
            std::cerr << "[BuildLab] failed to emit file " << (is_use_obj_format ? mod->object_path.data() : mod->bitcode_path.data()) << " " << std::endl;
        }
#endif

    }

    return compile_result;


}

int link_objects(const std::string& comp_exe_path, std::vector<chem::string>& linkables, const std::string& output_path) {
    int link_result;
#ifdef COMPILER_BUILD
    std::vector<std::string> data;
    for(auto& obj : linkables) {
        data.emplace_back(obj.data());
    }
    link_result = link_objects(data, output_path, comp_exe_path, {});
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
    return link_objects(options->exe_path, linkables, output_path);
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

int LabBuildCompiler::do_to_c_job(LabJob* job) {

    std::ofstream stream;
    stream.open(job->abs_path.data());
    if (!stream.is_open()) {
        std::cerr << rang::fg::red << "[2C] Failed to open path : " << job->abs_path.data() << rang::fg::reset << std::endl;
        return false;
    }

    // creating symbol resolver
    SymbolResolver resolver(true);

    // shrinking visitor used to shrink
    ShrinkingVisitor shrinker;

    // the processor that does everything
    ASTProcessor processor(
            options,
            &resolver
    );

    int compile_result = 0;

    // beginning
    ToCAstVisitor visitor(&stream);
    // TODO prepare c visitor from options

    // allow user the compiler (namespace) functions in @comptime
    visitor.comptime_scope.prepare_compiler_namespace(resolver);

    // preparing translation
    visitor.prepare_translate();

    // get flattened modules
    auto flattened_modules = flatten_dedupe_sorted(job->dependencies);

    std::vector<std::future<ASTImportResultExt>> futures;

    for(auto mod : flattened_modules) {

        // get flat imports
        auto flat_imports = processor.determine_mod_imports(mod);

        // parallel parsing
        futures.clear();
        int i = 0;
        for(const auto& file : flat_imports) {
            futures.push_back(pool.push(concurrent_processor, i, file, &processor));
            i++;
        }

        i = 0;
        for(const auto& file : flat_imports) {

            // importing
            auto result = futures[i].get();
            if(!result.continue_processing) {
                compile_result = 1;
                break;
            }

            // reset the visitor so it can be used for another file
            visitor.reset();

            // perform symbol resolution
            processor.sym_res(result.scope, result.is_c_file, file.abs_path);
            if (resolver.has_errors) {
                compile_result = 1;
                break;
            }
            resolver.reset_errors();

            // translating
            processor.translate_to_c(visitor, result.scope, file);

            // shrinking the nodes
            processor.shrink_nodes(shrinker, result.scope, file);

            i++;
        }
    }

    stream.close();
    processor.end();

    return compile_result;
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
        auto nodes = TranslateC(options->exe_path.data(), mod->paths[0].data(), options->get_resources_path().c_str());
        visitor.translate(nodes);
    }
    output.close();
    return 0;
#else
    std::cerr << "Translating c files to chemical can only be done by the compiler executable, please check using compiler::is_clang_based() in build.lab" << std::endl;
    return 1;
#endif
}

int LabBuildCompiler::build_lab_file(LabBuildContext& context, const std::string& path) {

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // creating symbol resolver for build.lab files only
    SymbolResolver lab_resolver(options->is64Bit);

    // the processor that does everything for build.lab files only
    ASTCompiler lab_processor(
        options,
        &lab_resolver
    );

    // get flat imports
    auto flat_imports = lab_processor.flat_imports(path);
    int compile_result = 0;

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor c_visitor(&output_ptr);

    // allow user the compiler (namespace) functions in @comptime
    c_visitor.comptime_scope.prepare_compiler_namespace(lab_resolver);

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
        std::vector<std::future<ASTImportResultExt>> lab_futures;
        int i = 0;
        for (const auto &file: flat_imports) {
            lab_futures.push_back(pool.push(concurrent_processor, i, file, &lab_processor));
            i++;
        }

        // processing each build.lab file and creating C output
        i = 0;
        for (const auto &file: flat_imports) {

            auto result = lab_futures[i].get();
            if (!result.continue_processing) {
                compile_result = false;
                break;
            }

            // print the benchmark or verbose output received from processing
            if((options->benchmark || options->verbose) && !result.cli_out.empty()) {
                std::cout << rang::style::bold << rang::fg::magenta << "[Processing] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
                std::cout << result.cli_out << std::flush;
            }

            // symbol resolution
            lab_processor.sym_res(result.scope, result.is_c_file, file.abs_path);
            if (lab_resolver.has_errors) {
                compile_result = false;
                break;
            }
            lab_resolver.reset_errors();

            // the last build.lab file is whose build method is to be called
            bool is_last = i == flat_imports.size() - 1;
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
                found->annotations.emplace_back(AnnotationKind::Api);
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
                    auto ns = new Namespace(file.as_identifier, nullptr);
                    for (auto &node: result.scope.nodes) {
                        node->set_parent(ns);
                    }
                    ns->nodes = std::move(result.scope.nodes);
                    result.scope.nodes.emplace_back(ns);
                }
            }

            // translate build.lab file to c
            lab_processor.translate_to_c(c_visitor, result.scope, file);

            // shrinking the nodes
            lab_processor.shrink_nodes(shrinker, result.scope, file);

            i++;
        }
    }

    lab_processor.end();

    // return if error occurred during processing of build.lab(s)
    if(compile_result == 1) {
        return compile_result;
    }

    // compiling the c output from build.labs
    auto str = output_ptr.str();
    auto state = compile_c_to_tcc_state(options->exe_path.data(), str.data(), "", true, is_debug(options->def_mode));

    if(state == nullptr) {
        const auto out_path = resolve_rel_child_path_str(context.build_dir, "build.lab.c");
        writeToFile(out_path, str);
        std::cerr << rang::fg::red << "[LabBuild] Couldn't build lab file due to error in translation, translated C written at " << out_path << rang::fg::reset << std::endl;
        return 1;
    }

    TCCDeletor auto_del(state); // automatic destroy

    // relocate the code before calling
    tcc_relocate(state);

    // get the build method
    auto build = (void(*)(BuildContextCBI*)) tcc_get_symbol(state, "build");
    if(!build) {
        std::cerr << "[LabBuild] Couldn't get build function symbol in translated c :\n" << str << std::endl;
        return 1;
    }

    // prepare the cbi
    BuildContextCBI cbi{};
    prep_build_context_cbi(&cbi);
    bind_build_context_cbi(&cbi, &context);

    // call the root build.lab build's function
    build(&cbi);

    // mkdir the build directory
    if(!std::filesystem::exists(context.build_dir)) {
        std::filesystem::create_directory(context.build_dir);
    }

    int job_result = compile_result;

    // generating outputs (executables)
    for(auto& exe : context.executables) {

        job_result = do_job(exe.get());
        if(job_result == 1) {
            std::cerr << rang::fg::red << "[BuildLab]" << " error performing job '" << exe->name.data() << "', returned status code 1" << rang::fg::reset << std::endl;
        }

    }

    // running the on_finished lambda
    if(context.on_finished) {
        context.on_finished(context.on_finished_data);
    }

    return job_result;

}