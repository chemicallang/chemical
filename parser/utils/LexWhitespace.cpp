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

void Parser::consumeWhitespaceAndNewLines() {
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