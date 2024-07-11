// Copyright (c) Qinetik 2024.

#include "IGCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "SymbolResolver.h"
#include "preprocess/ShrinkingVisitor.h"
#include "utils/PathUtils.h"
#include "ctpl.h"
#include "ASTCompiler.h"
#include <iostream>
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

bool compile(Codegen *gen, const std::string &path, IGCompilerOptions *options) {

    auto builddir = resolve_rel_parent_path_str(path, "build");

    // creating symbol resolver
    SymbolResolver resolver(options->is64Bit);

    // allow user the compiler (namespace) functions in @comptime
    gen->comptime_scope.prepare_compiler_namespace(resolver);

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // the processor that does everything
    ASTCompiler processor(
        options,
        &resolver
    );

    auto flat_imports = processor.flat_imports(path);

    // beginning
    gen->module_init("ChemMod");
    gen->compile_begin();

    bool compile_result = true;

    ctpl::thread_pool pool((int) std::thread::hardware_concurrency()); // Initialize thread pool with the number of available hardware threads
    std::vector<std::future<ASTImportResultExt>> futures;

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

        // compiling the nodes
        gen->nodes = std::move(result.scope.nodes);
        processor.compile_nodes(gen, shrinker, file);

        i++;
    }

    processor.end();
    gen->compile_end();

    if (gen->has_errors) {
        return false;
    }

    return compile_result;

}