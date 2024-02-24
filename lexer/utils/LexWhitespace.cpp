// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "lexer/Lexer.h"
#include "lexer/model/tokens/WhitespaceToken.h"
#include <memory>

int Lexer::lexWhitespace() {
    int whitespaces = 0;
    while (!provider.eof() && (provider.peek() == ' ' || provider.peek() == '\t')) {
        provider.readCharacter();
        whitespaces++;
    }
    return whitespaces;
}

void Lexer::lexWhitespaceToken(std::vector<std::unique_ptr<LexToken>> &tokens) {
    auto whitespace = lexWhitespace();
    if (whitespace > 0) {
        tokens.emplace_back(std::make_unique<WhitespaceToken>(provider.position() - whitespace, whitespace, lineNumber()));
    }
}
