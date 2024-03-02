// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "lexer/Lexi.h"
#include "parser/Persi.h"
#include "utils/Utils.h"

int main(int argc, char *argv[]) {
    if (argc == 0) {
        std::cout << "A file path argument is required so the file can be parsed";
        return 0;
    }
    auto lexer = benchLexFile(argv[1], LexConfig{});
    printTokens(lexer.tokens);
    for(const auto& err : lexer.errors) {
        std::cerr << "[Lexer] " << err.message << ' ' << err.position.formatted() << "\n";
    }
    auto parser = benchParse(std::move(lexer.tokens));
    for(const auto& err : parser.errors) {
        std::cerr << err;
    }
    Scope scope(std::move(parser.nodes));
    std::cout << "[Representation]\n" << scope.representation() << "\n";
    return 0;
}