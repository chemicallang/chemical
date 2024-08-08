// Copyright (c) Qinetik 2024.

#include "Codegen.h"
#include "preprocess/ShrinkingVisitor.h"
#include "utils/Benchmark.h"
#include "ASTCompiler.h"

void ASTCompiler::compile_nodes(
        Codegen* gen,
        ShrinkingVisitor& shrinker,
        const FlatIGFile &file
) {
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    gen->compile_nodes();
    if(options->benchmark) {
        bm_results->benchmark_end();
        std::cout << "[Compile] " << file.abs_path << " Completed " << bm_results->representation() << std::endl;
    }
    if(!gen->errors.empty()) {
        gen->print_errors(file.abs_path);
    }
    gen->reset_errors();
    if(options->shrink_nodes) {
        shrinker.visit(gen->nodes);
    }
    shrinked_nodes[file.abs_path] = std::move(gen->nodes);
}