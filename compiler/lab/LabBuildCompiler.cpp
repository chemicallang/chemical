// Copyright (c) Qinetik 2024.

#include "LabBuildCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
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
#include "compiler/LinkerUtils.h"
#include "utils/PathUtils.h"
#include "compiler/ASTCompiler.h"
#include <sstream>
#include <filesystem>
#include <iostream>
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

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

int lab_build(LabBuildContext& context, const std::string& path, LabBuildCompilerOptions* options) {

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
    ToCAstVisitor visitor(&output_ptr, path);

    // allow user the compiler (namespace) functions in @comptime
    visitor.comptime_scope.prepare_compiler_functions(lab_resolver);

    // preparing translation
    visitor.prepare_translate();

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

    // creating a thread pool for all our jobs in the lab build
    ctpl::thread_pool pool((int) std::thread::hardware_concurrency()); // Initialize thread pool with the number of available hardware threads

    {
        std::vector<std::future<ASTImportResult>> lab_futures;
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
            lab_processor.translate_to_c_no_sym_res(visitor, result.scope, shrinker, file);

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
    auto state = compile_c_to_tcc_state(options->exe_path.data(), str.data(), "", false);
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

    // all the objects that should be linked for each executable
    std::vector<std::string> linkables;
    int link_result;

    // mkdir the build directory
    if(!std::filesystem::exists(context.build_dir)) {
        std::filesystem::create_directory(context.build_dir);
    }

    // for each object file
    bool save_result;

    // the index into futures
    int i;

    // generating outputs (executables)
    for(auto& exe : context.executables) {

        std::string exe_build_dir = exe.build_dir.to_std_string();
        // create the build directory for this executable
        if(!std::filesystem::exists(exe_build_dir)) {
            std::filesystem::create_directory(exe_build_dir);
        }

        // a new symbol resolver for every executable
        SymbolResolver resolver(options->is64Bit);

        // prepare a global scope for all modules
        GlobalInterpretScope global_scope;
        global_scope.prepare_compiler_functions(resolver);

        // creating codegen for this executable
        Codegen gen({}, options->target_triple, options->exe_path, options->is64Bit, "");

        // the processor that does everything
        ASTCompiler processor(
                options,
                &resolver
        );

        // emitter options allow to configure type of build (debug or release)
        CodegenEmitterOptions emitter_options;
        configure_emitter_opts(options->def_mode, &emitter_options);
        if(options->def_lto_on) {
            emitter_options.lto = true;
        }
        if(options->def_assertions_on) {
            emitter_options.assertions_on = true;
        }

        // configure output path
        auto obj_path = resolve_rel_child_path_str(exe_build_dir, exe.name.to_std_string() + ".o");
        emitter_options.obj_path = obj_path.data();

        // compile dependent modules for this executable
        for(auto mod : exe.dependencies) {

            // get flat file map of this module
            flat_imports = processor.determine_mod_imports(mod);

            // send all files for concurrent processing (lex and parse)
            std::vector<std::future<ASTImportResult>> futures;
            i = 0;
            for(const auto& file : flat_imports) {
                futures.push_back(pool.push(concurrent_processor, i, file, &processor));
                i++;
            }

            // prepare for code generation of this module
            gen.module_init(mod->name.to_std_string());
            gen.compile_begin();

            // sequentially compile each file
            i = 0;
            for(const auto& file : flat_imports) {

                // get the processed result
                auto result = futures[i].get();
                if(!result.continue_processing) {
                    compile_result = false;
                    break;
                }

                // symbol resolution
                processor.sym_res(result.scope, result.is_c_file, file.abs_path);
                if (resolver.has_errors) {
                    compile_result = false;
                    break;
                }
                resolver.reset_errors();

                // compiling the nodes
                gen.nodes = std::move(result.scope.nodes);
                processor.compile_nodes(&gen, shrinker, file);

                i++;
            }

            processor.end();
            gen.compile_end();

            // errors in a single module means no linking
            if (gen.has_errors) {
                compile_result = 1;
                break;
            }

            // creating a object or bitcode file
            save_result = gen.save_with_options(&emitter_options);
            if(save_result) {
                linkables.emplace_back(obj_path);
            } else {
                std::cerr << "[BuildLab] failed to emit file " << obj_path << " " << std::endl;
            }

        }


        if(compile_result == 1) {
            break;
        }
        link_result = link_objects(linkables, exe.abs_path.to_std_string(), options->exe_path, {});
        if(link_result == 1) {
            std::cerr << "Failed to link" << std::endl;
            return link_result;
        }
        linkables.clear();

    }

    return compile_result;

}