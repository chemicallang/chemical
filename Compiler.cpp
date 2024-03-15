// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "lexer/Lexi.h"
#include "parser/Persi.h"
#include "utils/Utils.h"
#include "ast/utils/ExpressionEvaluator.h"
#include "ast/utils/ValueType.h"
#include "ast/base/GlobalInterpretScope.h"
#include "compiler/Codegen.h"

int main(int argc, char *argv[]) {
    if (argc == 0) {
        std::cout << "A file path argument is required so the file can be parsed";
        return 0;
    }
    auto lexer = benchLexFile(argv[1]);
//    printTokens(lexer.tokens);
    for (const auto &err: lexer.errors) {
        std::cerr << err.representation() << std::endl;
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for (const auto &err: parser.errors) {
        std::cerr << err;
    }
    TypeChecker checker(std::move(parser.nodes));
    checker.type_check();
    for(const auto &err : checker.errors) {
        std::cerr << err << std::endl;
    }
    Scope scope(std::move(checker.nodes));
//    std::cout << "[Representation]\n" << scope.representation() << std::endl;
    if(!lexer.errors.empty() || !parser.errors.empty() || !checker.errors.empty()) return 1;
    Codegen gen(std::move(scope.nodes), argv[1]);
    // compile
    gen.compile();
    // print the errors occurred
    for(const auto& error : gen.errors) {
        std::cout << error << std::endl;
    }
    // save the generation to a file
    gen.save_to_file("sample/compiled.ll");
    // print to console
    gen.print_to_console();

    return 0;
}