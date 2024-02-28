// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "lexer/Lexi.h"
#include "parser/Persi.h"

int main(int argc, char *argv[]) {
    if (argc == 0) {
        std::cout << "A file path argument is required so the file can be parsed";
        return 0;
    }
    auto parser = benchParse(benchLexFile(argv[1], LexConfig{}));
    if (parser.parseError.has_value()) {
        std::cerr << "Error parsing the file:" << parser.parseError.value();
    }
    return 0;
}