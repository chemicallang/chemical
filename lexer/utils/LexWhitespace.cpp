// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "lexer/Lexer.h"
#include "lexer/model/tokens/RawToken.h"
#include <memory>

bool Lexer::readWhitespace() {
    if(provider.peek() != ' ' && provider.peek() != '\t') return false;
    return provider.readWhitespaces() > 0;
}

bool Lexer::lexWhitespaceToken() {
    return readWhitespace();
}