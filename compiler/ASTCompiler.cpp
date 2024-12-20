// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include "preprocess/ShrinkingVisitor.h"
#include "utils/Benchmark.h"
#include "ASTProcessor.h"
#include "compiler/SymbolResolver.h"

void ASTProcessor::compile_nodes(
        Codegen& gen,
        std::vector<ASTNode*>& imported_generics,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    gen.compile_nodes(imported_generics);
    gen.compile_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(abs_path, "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::declare_nodes(
        Codegen& gen,
        std::vector<ASTNode*>& imported_generics,
        Scope& import_res,
        const std::string& abs_path
) {
    auto& nodes_vec = import_res.nodes;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    gen.compile_nodes(imported_generics);
    gen.declare_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(abs_path, "Compile");
        gen.diagnostics.clear();
    }
}