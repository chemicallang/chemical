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
#include "ShrinkingVisitor.h"
#include <sstream>

#ifdef COMPILER_BUILD

std::vector<std::unique_ptr<ASTNode>>
TranslateC(const char *exe_path, const char *abs_path, const char *resources_path);

#endif

bool translate(
        const std::string &path,
        std::ostream* output_ptr,
        ToCTranslatorOptions *options,
        const std::function<void(ToCAstVisitor*, ASTProcessor*)>& prepare
) {

    // creating symbol resolver
    SymbolResolver resolver(path, true);

    // shrinking visitor used to shrink
    ShrinkingVisitor shrinker;

    // the processor that does everything
    ASTProcessor processor(
            options,
            &resolver
    );

    // prepare
    processor.prepare(path);

    // get flat imports
    auto flat_imports = processor.flat_imports();

    bool compile_result = true;

    // beginning
    ToCAstVisitor visitor(output_ptr, path);
    prepare(&visitor, &processor);

    // allow user the compiler (namespace) functions in @comptime
    visitor.comptime_scope.prepare_compiler_functions(resolver);

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
        visitor.current_path = file.abs_path;
        visitor.translate(result.scope.nodes);
        if(options->shrink_nodes) {
            shrinker.visit(result.scope.nodes);
        }
        processor.file_nodes.emplace_back(std::move(result.scope.nodes));

    }

    processor.end();

    visitor.print_errors();

    return compile_result;

}

bool translate(
        const std::string &path,
        const std::string& output_path,
        ToCTranslatorOptions *options,
        const std::function<void(ToCAstVisitor*, ASTProcessor*)>& prepare
){
    std::ofstream stream;
    stream.open(output_path);
    if (!stream.is_open()) {
        std::cerr << "[2C] Failed to open path : " << output_path << std::endl;
        return false;
    }
    auto value = translate(
        path,
        &stream,
        options,
        prepare
    );
    stream.close();
    return value;
}

std::string translate(const std::string& path, ToCTranslatorOptions* options, const std::function<void(ToCAstVisitor*, ASTProcessor*)>& prepare) {
    std::stringstream stream;
    auto value = translate(
            path,
            &stream,
            options,
            prepare
    );
    if(value) {
        return stream.str();
    } else {
        return "";
    }
}