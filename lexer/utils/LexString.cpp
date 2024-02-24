// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/IdentifierToken.h"

std::string Lexer::lexAnything(char until) {
    std::string str;
    while (!provider.eof() && provider.peek() != until) {
        char x = provider.readCharacter();
        str.append(1, x);
    }
    return str;
}

std::string Lexer::lexAlphaNum() {
    std::string str;
    while (!provider.eof() && std::isalnum(provider.peek())) {
        str.append(1, provider.readCharacter());
    }
    return str;
}

std::string Lexer::lexIdentifierToken(std::vector<std::unique_ptr<LexToken>> &tokens) {
    auto id = lexAlphaNum();
    if(!id.empty()) {
        tokens.emplace_back(std::make_unique<IdentifierToken>(backPosition(id.length()), id));
        return id;
    } else {
        return id;
    }
}