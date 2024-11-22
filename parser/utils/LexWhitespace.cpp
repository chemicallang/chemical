// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "parser/Parser.h"
#include <memory>

bool Parser::readWhitespace() {
    const auto c = provider.peek();
    if(c != ' ' && c != '\t') return false;
    return provider.readWhitespaces() > 0;
}

bool Parser::lexWhitespaceToken() {
    return readWhitespace();
}