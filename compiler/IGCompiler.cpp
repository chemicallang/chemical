// Copyright (c) Qinetik 2024.

#include "IGCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "SymbolResolver.h"
#include "ASTProcessor.h"
#include "preprocess/ShrinkingVisitor.h"
#include "utils/PathUtils.h"
#include "ctpl.h"
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
    gen->comptime_scope.prepare_compiler_functions(resolver);

    // shrinking visitor will shrink everything
    ShrinkingVisitor shrinker;

    // the processor that does everything
    ASTProcessor processor(
        options,
        &resolver
    );

    // prepare
    processor.prepare(path);

    LabBuildContext context;
    auto root_module = processor.get_root_module(context, path);
    auto flat_modules = LabBuildContext::flatten_dedupe_sorted(root_module);

    // beginning
    gen->compile_begin();

    bool compile_result = true;

    ctpl::thread_pool pool((int) std::thread::hardware_concurrency()); // Initialize thread pool with the number of available hardware threads
    std::vector<std::future<ASTImportResult>> futures;

    for(auto module : flat_modules) {

        auto bc_file_loc = resolve_rel_child_path_str(builddir, module->name.to_std_string() + ".bc");
        gen->module_init(module->name.to_std_string());
        futures.clear();

        auto flat_imports = processor.determine_mod_imports(module);

        // TODO if contents of files have changed since we generated the bc file last time

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
            std::unique_ptr<BenchmarkResults> bm_results;
            if(options->benchmark) {
                bm_results = std::make_unique<BenchmarkResults>();
                bm_results->benchmark_begin();
            }
            gen->compile_nodes();
            if(options->benchmark) {
                bm_results->benchmark_end();
                std::cout << std::endl << "[Compile] " << file.abs_path << " Completed " << bm_results->representation() << std::endl;
            }
            if(options->shrink_nodes) {
                shrinker.visit(gen->nodes);
            }
            processor.file_nodes.emplace_back(std::move(gen->nodes));
            if(!gen->errors.empty()) {
                gen->print_errors(file.abs_path);
                std::cout << std::endl;
            }
            gen->reset_errors();

            i++;
        }

        if(flat_modules.size() != 1) {
            gen->save_to_bc_file(bc_file_loc, options->mode);
        }

    }

    processor.end();

    gen->compile_end();

    if (gen->has_errors) {
        return false;
    }

    return compile_result;

}