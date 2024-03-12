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
    printTokens(lexer.tokens);
    for (const auto &err: lexer.errors) {
        std::cerr << err.representation() << std::endl;
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for (const auto &err: parser.errors) {
        std::cerr << err;
    }
//    std::cout << "[Representation]\n" << scope.representation() << "\n";

    Codegen gen(std::move(parser.nodes), argv[1]);

    // save the generation to a file
    gen.save_to_file("sample/compiled.ll");

    // TODO interpret

    return 0;
}