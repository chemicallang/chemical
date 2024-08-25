// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include "preprocess/ShrinkingVisitor.h"
#include "utils/Benchmark.h"
#include "ASTCompiler.h"
#include "compiler/SymbolResolver.h"

void ASTCompiler::compile_nodes(
        Codegen* gen,
        Scope& import_res,
        const FlatIGFile &file
) {
    gen->nodes = std::move(import_res.nodes);
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    std::vector<ASTNode*> imported_generics;
    imported_generics.reserve(resolver->imported_generic.size());
    for(auto& node : resolver->imported_generic) {
        imported_generics.emplace_back(node.first);
    }
    gen->compile_nodes(imported_generics);
    gen->compile_nodes();
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", bm_results.get());
    }
    if(!gen->errors.empty()) {
        gen->print_errors(file.abs_path);
        gen->errors.clear();
    }
    import_res.nodes = std::move(gen->nodes);
}