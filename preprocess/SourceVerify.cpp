// Copyright (c) Qinetik 2024.

#include "SourceVerifier.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include "compiler/ASTProcessor.h"
#include "ShrinkingVisitor.h"
#include "ctpl.h"
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

bool verify(const std::string &path, SourceVerifierOptions *options) {


    // creating symbol resolver
    SymbolResolver resolver(true);

    // shrinking visitor
    ShrinkingVisitor shrinker;

    // the processor that does everything
    ASTProcessor processor(
            options,
            &resolver
    );

    // get flat imports
    auto flat_imports = processor.flat_imports(path);

    bool compile_result = true;

    ctpl::thread_pool pool((int) std::thread::hardware_concurrency()); // Initialize thread pool with the number of available hardware threads
    std::vector<std::future<ASTImportResult>> futures;
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

        // storing nodes
        if(options->shrink_nodes) {
            shrinker.visit(result.scope.nodes);
        }
        processor.file_nodes.emplace_back(std::move(result.scope.nodes));

        i++;
    }

    processor.end();

    return compile_result;

}