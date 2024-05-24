// Copyright (c) Qinetik 2024.

#include "IGCompiler.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "SymbolResolver.h"
#include "ASTProcessor.h"
#include <iostream>
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

IGCompilerOptions::IGCompilerOptions(
        std::string exe_path,
        std::string target_triple,
        bool is64Bit
) : ASTProcessorOptions(std::move(exe_path)),
    target_triple(std::move(target_triple)), is64Bit(is64Bit) {

}

bool compile(Codegen *gen, const std::string &path, IGCompilerOptions *options) {

    // creating the lexer
    std::fstream file_stream;
    SourceProvider provider(file_stream);
    Lexer lexer(provider, path);

    // the cst converter
    CSTConverter converter(options->is64Bit);
    converter.no_imports = true;

    // creating symbol resolver
    SymbolResolver resolver(options->exe_path, path, options->is64Bit);

    // the processor that does everything
    ASTProcessor processor(
        options,
        &lexer,
        &converter,
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