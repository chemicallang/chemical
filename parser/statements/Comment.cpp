// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"

bool Parser::lexSingleLineCommentTokens() {
    if(token->type == TokenType::SingleLineComment) {
        token++;
        emplace(LexTokenType::Comment, token->position, std::string(token->value));
        return true;
    } else {
        return false;
    }
}

bool Parser::lexMultiLineCommentTokens() {
    // TODO improve this function
    if(token->type == TokenType::MultiLineComment) {
        auto& pos = token->position;
        std::string value;
        while(token->type == TokenType::MultiLineComment) {
            value.append(token->value);
            token++;
        }
        emplace(LexTokenType::MultilineComment, pos, value);
        return true;
    } else {
        return false;
    }
}