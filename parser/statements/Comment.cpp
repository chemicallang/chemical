// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Comment.h"

Comment* Parser::parseSingleLineComment(ASTAllocator& allocator) {
    if(token->type == TokenType::SingleLineComment) {
        token++;
        return new (allocator.allocate<Comment>()) Comment(std::string(token->value), false, parent_node, loc_single(token));
    } else {
        return nullptr;
    }
}

Comment* Parser::parseMultiLineComment(ASTAllocator& allocator) {
    // TODO improve this function
    if(token->type == TokenType::MultiLineComment) {
        auto& pos = token->position;
        Token* last_token = token;
        std::string value;
        while(token->type == TokenType::MultiLineComment) {
            value.append(token->value);
            last_token = token;
            token++;
        }
        return new (allocator.allocate<Comment>()) Comment(value, false, parent_node, loc(pos, end_pos(last_token)));
    } else {
        return nullptr;
    }
}