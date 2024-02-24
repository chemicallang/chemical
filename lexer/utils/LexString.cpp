// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"

std::string Lexer::lexString() {
    std::string str;
    while (!provider.eof() && provider.peek() != ' ') {
        char x = provider.readCharacter();
        str.append(1, x);
    }
    return str;
}