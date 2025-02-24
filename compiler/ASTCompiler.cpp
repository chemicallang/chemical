// Copyright (c) Chemical Language Foundation 2025.

#include "Codegen.h"
#include "utils/Benchmark.h"
#include "ASTProcessor.h"
#include "compiler/SymbolResolver.h"
#include "rang.hpp"

void ASTProcessor::code_gen_declare(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    // create a di compile unit
    gen.di.createDiCompileUnit(chem::string_view(abs_path.data(), abs_path.size()));
    gen.declare_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Declare", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(abs_path, "Declare");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_compile(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    // create a di compile unit
    gen.compile_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", abs_path, bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(abs_path, "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::declare_and_compile(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    // create a di compile unit
    gen.di.createDiCompileUnit(chem::string_view(abs_path.data(), abs_path.size()));
    gen.declare_and_compile(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(abs_path, "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::external_declare_nodes(
        Codegen& gen,
        Scope& import_res,
        const std::string& abs_path
) {
    auto& nodes_vec = import_res.nodes;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    gen.di.createDiCompileUnit(chem::string_view(abs_path.data(), abs_path.size()));
    gen.external_declare_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Declare", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(abs_path, "Declare");
        gen.diagnostics.clear();
    }
}

int ASTProcessor::compile_module(
    Codegen& gen,
    LabModule* module,
    std::vector<ASTFileResult*>& files
) {

    // first loop will only handle files that have been already compiled
    // meaning they have been imported from other modules (which have been compiled)
    // since we already have compiled them (function bodies compiled), now we just declare them (only prototypes of functions and structs)
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        auto present_unit = shrinked_unit.find(file.abs_path);
        if(present_unit == shrinked_unit.end()) {
            // not external module file
            continue;
        }

        ASTUnit& unit = present_unit->second;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[ExtDeclare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

        // do not continue processing
        if(!result.continue_processing) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during lexing or parsing file '" << file.abs_path << '\'' << rang::fg::reset << std::endl;
            return 1;
        }

        auto declared_in = unit.declared_in.find(module);
        if(declared_in == unit.declared_in.end()) {
            // this is probably a different module, so we'll declare the file (if not declared)
            external_declare_nodes(gen, unit.scope, file.abs_path);
            unit.declared_in[module] = true;
        }

        // clear everything we allocated using file allocator to make it re-usable
        safe_clear_file_allocator();

    }

    // The second loop deals with files that are within current module
    // We just declare the files in current module in this loop that's it, we create prototypes of structs and functions
    // in the next loop we will finally create bodies
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        // check file exists
        if(file.abs_path.empty()) {
            std::cerr << rang::fg::red << "error: file not found '" << file.import_path << "'" << rang::fg::reset << std::endl;
            return 1;
        }

        if(!result.read_error.empty()) {
            std::cerr << rang::fg::red << "error: when reading file '" << file.abs_path << "' with message '" << result.read_error << "'" << rang::fg::reset << std::endl;
            return 1;
        }

        if(shrinked_unit.find(file.abs_path) != shrinked_unit.end()) {
            // external module file
            continue;
        }

        ASTUnit& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
            print_results(result, file.abs_path, options->benchmark);
        }

        // do not continue processing
        if(!result.continue_processing) {
            std::cerr << rang::fg::red << "couldn't perform job due to errors during lexing or parsing file '" << file.abs_path << '\'' << rang::fg::reset << std::endl;
            return 1;
        }

        // compiling the nodes
        code_gen_declare(gen, unit.scope.nodes, file.abs_path);

        // clear everything we allocated using file allocator to make it re-usable
        safe_clear_file_allocator();

    }

    // The third loop also only compiles the files that present inside this module
    // This loop will compile the bodies of the functions inside the current module
    for(auto file_ptr : files) {

        auto& file = *file_ptr;
        auto& result = file;

        if(shrinked_unit.find(file.abs_path) != shrinked_unit.end()) {
            // external module file
            continue;
        }

        ASTUnit& unit = file.unit;
        // compiling the nodes
        code_gen_compile(gen, unit.scope.nodes, file.abs_path);
        // save the file result, for future retrievals
        shrinked_unit[file.abs_path] = std::move(result.unit);

        // clear everything we allocated using file allocator to make it re-usable
        safe_clear_file_allocator();

    }

    return 0;

}