// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/WhileLoop.h"
#include "ast/values/NullValue.h"

WhileLoop* fix_loop(WhileLoop* loop, ASTAllocator& allocator) {
    if(!loop->condition) {
        loop->condition = new (allocator.allocate<NullValue>()) NullValue(nullptr, ZERO_LOC);
    }
    return loop;
}

WhileLoop* Parser::parseWhileLoop(ASTAllocator& allocator) {

    auto& tok = *token;
    if(tok.type != TokenType::WhileKw) {
        return nullptr;
    }

    token++;

    auto loop = new (allocator.allocate<WhileLoop>()) WhileLoop(nullptr, parent_node, loc_single(tok));

    if(!consumeToken(TokenType::LParen)) {
        unexpected_error("expected a starting parenthesis ( after keyword while for while block");
        return fix_loop(loop, allocator);
    }

    auto expr = parseExpression(allocator);
    if(expr) {
        loop->condition = expr;
    } else {
        unexpected_error("expected a conditional statement for while block");
        return fix_loop(loop, allocator);
    }

    if(!consumeToken(TokenType::RParen)) {
        unexpected_error("expected a closing parenthesis ) for while block");
        return loop;
    }

    // { statement(s) } with continue & break support
    auto block = parseBraceBlockOrSingleStmt("whileloop", loop, allocator);
    if(block.has_value()) {
        auto& blk = block.value();
        loop->body.nodes = std::move(blk.nodes);
        loop->body.set_parent(blk.parent());
    } else {
        unexpected_error("expected a brace block { statement(s) } when lexing a while block");
        return loop;
    }

    return loop;

}