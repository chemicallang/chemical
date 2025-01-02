// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/Comment.h"

Comment* Parser::parseSingleLineComment(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::SingleLineComment) {
        token++;
        return new (allocator.allocate<Comment>()) Comment(t.value.str(), false, parent_node, loc_single(t));
    } else {
        return nullptr;
    }
}

Comment* Parser::parseMultiLineComment(ASTAllocator& allocator) {
    auto& t = *token;
    if(t.type == TokenType::MultiLineComment) {
        token++;
        return new (allocator.allocate<Comment>()) Comment(t.value.str(), true, parent_node, loc_single(t));
    } else {
        return nullptr;
    }
}