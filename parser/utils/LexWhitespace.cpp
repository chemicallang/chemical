// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "parser/Parser.h"
#include <memory>

bool Parser::readWhitespace() {
    if(token->type == TokenType::Whitespace) {
        token++;
        return true;
    } else {
        return false;
    }
}

bool Parser::lexWhitespaceToken() {
    // we do not store the whitespace token
    // but we may for formatting one day
    return readWhitespace();
}

void Parser::lexWhitespaceAndNewLines() {
    while(true) {
        switch(token->type) {
            // case TokenType::SingleLineComment:
            // case TokenType::MultiLineComment:
            case TokenType::Whitespace:
            case TokenType::NewLine:
                token++;
                break;
            default:
                return;
        }
    }
}