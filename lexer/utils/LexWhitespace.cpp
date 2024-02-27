// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "lexer/Lexer.h"
#include "lexer/model/tokens/WhitespaceToken.h"
#include <memory>

unsigned int Lexer::lexWhitespace() {
    unsigned int whitespaces = 0;
    while (!provider.eof() && (provider.peek() == ' ' || provider.peek() == '\t')) {
        provider.readCharacter();
        whitespaces++;
    }
    return whitespaces;
}

bool Lexer::lexWhitespaceToken() {
    auto whitespace = lexWhitespace();
    if (whitespace > 0) {
        if (shouldAddWhitespaceToken()) {
            tokens.emplace_back(std::make_unique<WhitespaceToken>(backPosition(whitespace), whitespace));
        }
        return true;
    } else {
        return false;
    }
}

void Lexer::lexWhitespaceAndNewLines() {
    while (!provider.eof()) {
        if (!lexNewLineChars() && !lexWhitespaceToken()) {
            break;
        }
    }
}