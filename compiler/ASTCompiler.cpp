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
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Declare", &bm_results);
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
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.compile_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Compile", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "Compile");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement_declare(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_implement_declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "ExtDeclare", abs_path, &bm_results);
    }
    if(!gen.diagnostics.empty()) {
        gen.print_diagnostics(chem::string_view(abs_path), "ExtDeclare");
        gen.diagnostics.clear();
    }
}

void ASTProcessor::code_gen_external_implement(
        Codegen& gen,
        std::vector<ASTNode*>& nodes_vec,
        const std::string_view& abs_path
) {
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_implement_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "ExtCompile", abs_path, &bm_results);
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
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
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
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Compile", &bm_results);
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
    BenchmarkResults bm_results;
    if(options->benchmark_files) {
        bm_results.benchmark_begin();
    }
    gen.external_declare_nodes(nodes_vec);
    if(options->benchmark_files) {
        bm_results.benchmark_end();
        print_benchmarks(std::cout, "Declare", &bm_results);
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

    // creating a compile unit
    const auto compileUnit = gen.di.createDiCompileUnit(module->direct_files.empty() ? chem::string_view("") : chem::string_view(module->direct_files.front().abs_path));
    gen.di.start_di_compile_unit(compileUnit);

    // we will declare the direct dependencies of this module
    for(const auto dep : dependencies) {
        for(auto& metaFile : dep->direct_files) {

            auto& file = *metaFile.result;
            auto& body = file.unit.scope.body;
            auto& abs_path = file.abs_path;

            if (gen.di.isEnabled) {

                // start the file scope
                gen.di.start_file_scope(metaFile.result);

                // declare external nodes
                external_declare_nodes(gen, body, file.abs_path);

                // end the file scope
                gen.di.end_file_scope();

            } else {

                // declare external nodes
                external_declare_nodes(gen, body, file.abs_path);

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

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(&result);

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // compiling the nodes
            code_gen_declare(gen, unit.scope.body.nodes, abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // these are the generically monomorphized instantiations
    // that were created by this module
    auto& instantiations = container.get_current_module_instantiations();

    // we track for which files we created new di units
    // we must set them to null (so they don't get reused)
    std::vector<ASTFileResult*> newly_created_di_units;

    // declaring the generic instantiations created in this module
    for (const auto node : instantiations) {
        const auto file_scope = node->get_file_scope();
        if (file_scope == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file scope from a generically monomorphized declaration");
#endif
            continue;
        }
        const auto result = file_scope->meta.result;
        if (result == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file result from a generically monomorphized declaration");
#endif
            continue;
        }

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(result);

            // just declare
            node->code_gen_declare(gen);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // just declare
            node->code_gen_declare(gen);

        }

    }

    // implementing the generic instantiations created in this module
    for (const auto node : instantiations) {
        const auto file_scope = node->get_file_scope();
        if (file_scope == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file scope from a generically monomorphized declaration");
#endif
            continue;
        }
        const auto result = file_scope->meta.result;
        if (result == nullptr) {
#ifdef DEBUG
            CHEM_THROW_RUNTIME("couldn't get file result from a generically monomorphized declaration");
#endif
            continue;
        }

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(result);

            // implement the node
            node->code_gen(gen);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // implement the node
            node->code_gen(gen);

        }

    }

    // The fourth loop also only compiles the files that present inside this module
    // This loop will compile the bodies of the functions inside the current module
    for(auto& file_ptr : module->direct_files) {

        auto& file = *file_ptr.result;
        auto& result = file;

        ASTUnit& unit = file.unit;

        if (gen.di.isEnabled) {

            // start the file scope
            gen.di.start_file_scope(&result);

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

            // end the file scope
            gen.di.end_file_scope();

        } else {

            // compiling the nodes
            code_gen_compile(gen, unit.scope.body.nodes, file.abs_path);

        }

        // clear everything we allocated using file allocator to make it re-usable
        file_allocator.clear();

    }

    // end the di compile unit
    // this causes struct types to die
    gen.di.end_di_compile_unit();
    // this is really important
    // it removes cached types, cached scopes
    // if this is not called, freed memory can make into next module's compilation
    gen.di.finalize();
    // clear the generic instantiations for this module in the container
    // so these don't get generated when generating code for the next module
    container.clear_current_module_instantiations();
    // here we store struct types, function callee values to reuse them inside module
    // clearing this after module has compiled prevents reusing function callee's created
    // for this module, forces us to redeclare functions and structs in external module
    gen.mod_ptr_cache.clear();
    // here we store pointers (function callee values) for functions in different implementations
    // llvm pointers for functions inside impl blocks (that implement interfaces for structs) are stored here
    // clearing this has the same advantage of preventing reuse
    gen.trait_impl_func_map.clear();

    return 0;

}