// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//
#include "parser/Parser.h"
#include <memory>

void Parser::consumeNewLines() {
    while(true) {
        switch(token->type) {
            case TokenType::NewLine:
                token++;
                break;
            default:
                return;
        }
    }
}