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
#include "ctpl.h"
#include <iostream>
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

bool compile(Codegen *gen, const std::string &path, IGCompilerOptions *options) {

    // creating symbol resolver
    SymbolResolver resolver(path, options->is64Bit);

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

    // beginning
    gen->compile_begin();

    // get flat imports
    auto flat_imports = processor.flat_imports();

    bool compile_result = true;

    for(const auto& file : flat_imports) {

        auto result = processor.import_file(file);
        if(!result.continue_processing) {
            compile_result = false;
            break;
        }

        // compiling the nodes
        gen->current_path = file.abs_path;
        gen->nodes = std::move(result.scope.nodes);
        gen->compile_nodes();
        if(options->shrink_nodes) {
            shrinker.visit(gen->nodes);
        }
        processor.file_nodes.emplace_back(std::move(gen->nodes));

    }

    processor.end();

    if (!gen->errors.empty()) {
        gen->print_errors();
        std::cout << std::endl;
    }

    gen->compile_end();

    if (gen->has_errors) {
        return false;
    }

    return compile_result;

}