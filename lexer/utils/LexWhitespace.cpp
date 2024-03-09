// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "lexer/Lexer.h"
#include "lexer/model/tokens/WhitespaceToken.h"
#include <memory>

unsigned int Lexer::readWhitespaces() {
    unsigned int whitespaces = 0;
    while (!provider.eof() && (provider.peek() == ' ' || provider.peek() == '\t')) {
        provider.readCharacter();
        whitespaces++;
    }
    return whitespaces;
}

bool Lexer::lexWhitespaceToken() {
    if(provider.peek() != ' ' && provider.peek() != '\t') return false;
    auto whitespace = readWhitespaces();
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
        if(!(lexNewLineChars() || lexWhitespaceToken())){
            return;
        }
    }
}