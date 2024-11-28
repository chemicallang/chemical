// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <memory>
#include "parser/Parser.h"

// TODO remove this function
bool Parser::lexUnsignedIntAsNumberToken() {
    return lexNumberToken();
}

bool Parser::lexNumberToken() {
    if(token->type == TokenType::Number) {
        emplace(LexTokenType::Number, token->position, std::string(token->value));
        token++;
        return true;
    } else {
        return false;
    }
}