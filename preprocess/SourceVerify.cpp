// Copyright (c) Qinetik 2024.

#include "SourceVerifier.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include "compiler/ASTProcessor.h"
#include <utility>
#include <functional>

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

bool verify(const std::string &path, SourceVerifierOptions *options) {


    // creating the lexer
    std::fstream file_stream;
    SourceProvider provider(&file_stream);
    Lexer lexer(provider, path);

    // the cst converter
    CSTConverter converter(true);
    converter.no_imports = true;

    // creating symbol resolver
    SymbolResolver resolver(options->exe_path, path, true);

    // the processor that does everything
    ASTProcessor processor(
            options,
            &lexer,
            &converter,
            &resolver
    );

    // prepare
    processor.prepare(path);

    // get flat imports
    auto flat_imports = processor.flat_imports();

    bool compile_result = true;

    for(const auto& file : flat_imports) {

        // importing
        auto result = processor.import_file(file);
        if(!result.continue_processing) {
            compile_result = false;
            break;
        }

        // storing nodes
        processor.file_nodes.emplace_back(std::move(result.scope.nodes));

    }

    processor.end();

    return compile_result;

}