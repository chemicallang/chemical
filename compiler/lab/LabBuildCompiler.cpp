// Copyright (c) Qinetik 2024.

#include "LabBuildCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include "compiler/ASTProcessor.h"
#include "preprocess/ShrinkingVisitor.h"
#include "preprocess/2cASTVisitor.h"
#include "compiler/lab/LabBuildContext.h"
#include "integration/ide/bindings/BuildContextCBI.h"
#include "integration/libtcc/LibTccInteg.h"
#include "ctpl.h"
#include <sstream>
#include <iostream>
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

bool lab_build(LabBuildContext& context, const std::string& path, LabBuildCompilerOptions* options) {

    // creating symbol resolver
    SymbolResolver resolver(path, options->is64Bit);

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // the processor that does everything
    ASTProcessor processor(
        options,
        &resolver
    );

    // prepare
    processor.prepare(path);

    // get flat imports
    auto flat_imports = processor.flat_imports();

    bool compile_result = true;

    // beginning
    std::stringstream output_ptr;
    ToCAstVisitor visitor(&output_ptr, path);

    // allow user the compiler (namespace) functions in @comptime
    visitor.comptime_scope.prepare_compiler_functions(resolver);

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

    ctpl::thread_pool pool((int) std::thread::hardware_concurrency()); // Initialize thread pool with the number of available hardware threads
    std::vector<std::future<ASTImportResult>> futures;
    int i = 0;
    for(const auto& file : flat_imports) {
        futures.push_back(pool.push(concurrent_processor, i, file, &processor));
        i++;
    }

    i = 0;
    for(const auto& file : flat_imports) {

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

        // the last build.lab file is whose build method is to be called
        bool is_last = i == flat_imports.size() - 1;
        if(is_last) {
            auto found = finder(resolver, file.abs_path);
            if(found) {
                found->annotations.emplace_back(AnnotationKind::Api);
            } else {
                return false;
            }
        } else if(file.abs_path.ends_with(".lab")) {
            auto found = finder(resolver, file.abs_path, false);
            if(found) {
                // empty function names and anonymous means, 2c translator will replace the name, as if it's a lambda
                // this allows only a single build method, in root .lab file, all other build methods are c static and mangled function names
                found->name = "";
                found->annotations.emplace_back(AnnotationKind::Anonymous);
            }
        }

        processor.translate_to_c_no_sym_res(visitor, result.scope, shrinker, file);

        i++;
    }

    processor.end();

    auto str = output_ptr.str();
    auto state = compile_c_to_tcc_state(options->exe_path.data(), str.data(), "", false);
    TCCDeletor auto_del(state); // automatic destroy

    auto build = (LabModule*(*)(BuildContextCBI*)) tcc_get_symbol(state, "build");
    if(!build) {
        std::cerr << "[LabBuild] Couldn't get build function symbol in translated c :\n" << str << std::endl;
        return 1;
    }

    BuildContextCBI cbi;

    prep_build_context_cbi(&cbi);
    bind_build_context_cbi(&cbi, &context);

    // build and set the pointer to root module
    context.root_module = build(&cbi);

    return compile_result;

}