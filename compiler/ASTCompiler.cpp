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
    CP_UTF8;
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    gen.declare_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Declare", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Declare");
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
    gen.compile_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", abs_path, bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    std::unique_ptr<BenchmarkResults> bm_results;
    if(options->benchmark) {
        bm_results = std::make_unique<BenchmarkResults>();
        bm_results->benchmark_begin();
    }
    gen.external_implement_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "ExtCompile", abs_path, bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "ExtCompile");
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
    if(gen.di.isEnabled) {
        const auto compileUnit = gen.di.createDiCompileUnit(chem::string_view(abs_path.data(), abs_path.size()));
        gen.di.start_di_compile_unit(compileUnit);
        gen.declare_and_compile(nodes_vec);
        gen.di.end_di_compile_unit();
    } else {
        gen.declare_and_compile(nodes_vec);
    }
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Compile", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
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
    gen.external_declare_nodes(nodes_vec);
    if(options->benchmark) {
        bm_results->benchmark_end();
        print_benchmarks(std::cout, "Declare", bm_results.get());
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Declare");
        gen.diagnostics.clear();
    }
}

int ASTProcessor::compile_module(
    Codegen& gen,
    LabModule* module
) {

    // let's create a flat vector of direct dependencies, that we want to process
    std::vector<LabModule*> dependencies;
    shallow_dedupe_sorted(dependencies, module->dependencies);

    // we will declare the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& metaFile : dep->direct_files) {
            if(metaFile.result != nullptr) {
                auto& file = *metaFile.result;
                auto& body = file.unit.scope.body;
                auto& abs_path = file.abs_path;

                // this is probably a different module, so we'll declare the file (if not declared)
                if(gen.di.isEnabled) {

                    // we create the file di compile unit again
                    // that's because every module has a single di builder
                    // when the module completes it dies along with compile units
                    // so we create it again
                    const auto abs_path_view = chem::string_view(abs_path.data(), abs_path.size());
                    const auto fileDiCompileUnit = gen.di.createDiCompileUnit(abs_path_view);
                    file.diCompileUnit = fileDiCompileUnit;

                    // start the di compile unit
                    gen.di.start_di_compile_unit(fileDiCompileUnit);

                    // declare external nodes
                    external_declare_nodes(gen, body, file.abs_path);

                    // end the di compile unit scope
                    gen.di.end_di_compile_unit();

                } else {

                    // declare external nodes
                    external_declare_nodes(gen, body, file.abs_path);

                }

            } else {
                throw std::runtime_error("result is null");
            }
        }
    }

    // The second loop deals with files that are within current module
    // We just declare the files in current module in this loop that's it, we create prototypes of structs and functions
    // in the next loop we will finally create bodies
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        // print the benchmark or verbose output received from processing
        if((options->benchmark || options->verbose) && !empty_diags(result)) {
            std::cout << rang::style::bold << rang::fg::magenta << "[Declare] " << file.abs_path << rang::fg::reset << rang::style::reset << '\n';
        }

        auto& abs_path = file.abs_path;

        if(gen.di.isEnabled) {

            // create  di compile unit and attach it to the file
            const auto abs_path_view = chem::string_view(abs_path.data(), abs_path.size());
            const auto fileDiCompileUnit = gen.di.createDiCompileUnit(abs_path_view);

            // attach the unit to file
            file.diCompileUnit = fileDiCompileUnit;

            // start the unit
            gen.di.start_di_compile_unit(fileDiCompileUnit);

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

            // end the current unit
            gen.di.end_di_compile_unit();

        } else {

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // we will implement the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& metaFile : dep->direct_files) {
            if(metaFile.result != nullptr) {
                auto& file = *metaFile.result;
                auto& body = file.unit.scope.body;

                // this is probably a different module, so we'll declare the file (if not declared)
                if(gen.di.isEnabled) {

                    // start the di compile unit
                    gen.di.start_di_compile_unit(file.diCompileUnit);

                    // declare external nodes
                    code_gen_external_implement(gen, body.nodes, file.abs_path);

                    // end the di compile unit scope
                    gen.di.end_di_compile_unit();

                } else {

                    // declare external nodes
                    code_gen_external_implement(gen, body.nodes, file.abs_path);

                }

            } else {
                throw std::runtime_error("result is null");
            }
        }
    }

    // The fourth loop also only compiles the files that present inside this module
    // This loop will compile the bodies of the functions inside the current module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        if(gen.di.isEnabled) {

            // start the unit
            gen.di.start_di_compile_unit(file.diCompileUnit);

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

            // end the current unit
            gen.di.end_di_compile_unit();

        } else {

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // here we store struct types, function callee values to reuse them inside module
    // clearing this after module has compiled prevents reusing function callee's created
    // for this module, forces us to redeclare functions and structs in external module
    gen.mod_ptr_cache.clear();

    return 0;

}