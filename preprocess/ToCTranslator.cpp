// Copyright (c) Qinetik 2024.

#include "ToCTranslator.h"
#include "preprocess/ImportGraphMaker.h"
#include "utils/Benchmark.h"
#include "compiler/Codegen.h"
#include "lexer/Lexi.h"
#include "utils/Utils.h"
#include "cst/base/CSTConverter.h"
#include "compiler/SymbolResolver.h"
#include <iostream>
#include <utility>
#include <functional>
#include <ostream>
#include "2cASTVisitor.h"
#include "compiler/ASTProcessor.h"

#ifdef COMPILER_BUILD

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

#endif

ToCTranslatorOptions::ToCTranslatorOptions(
        std::string exe_path,
        std::string output_path,
        bool is64Bit
) : ASTProcessorOptions(std::move(exe_path)),
    output_path(std::move(output_path)),
    is64Bit(is64Bit) {

}

bool translate(const std::string &path, ToCTranslatorOptions *options) {

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

    // beginning
    std::ofstream stream;
    stream.open(options->output_path);
    if(!stream.is_open()) {
        std::cerr << "[2C] Failed to open path : " << options->output_path << std::endl;
        return false;
    }
    ToCAstVisitor visitor(stream);

    // preparing translation
    visitor.prepare_translate();

    for(const auto& file : flat_imports) {

        // importing
        auto result = processor.import_file(file);
        if(!result.continue_processing) {
            compile_result = false;
            break;
        }

        // translating the nodes
        visitor.translate(result.scope.nodes);
        processor.file_nodes.emplace_back(std::move(result.scope.nodes));

    }

    processor.end();
    stream.close();

    return compile_result;

}