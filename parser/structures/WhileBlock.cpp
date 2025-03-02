// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/WhileLoop.h"

WhileLoop* Parser::parseWhileLoop(ASTAllocator& allocator) {

    auto& tok = *token;
    if(tok.type != TokenType::WhileKw) {
        return nullptr;
    }

    token++;

    auto loop = new (allocator.allocate<WhileLoop>()) WhileLoop(nullptr, parent_node, loc_single(tok));

    if(!consumeToken(TokenType::LParen)) {
        error("expected a starting parenthesis ( after keyword while for while block");
        return loop;
    }

    auto expr = parseExpression(allocator);
    if(expr) {
        loop->condition = expr;
    } else {
        error("expected a conditional statement for while block");
        return loop;
    }

    if(!consumeToken(TokenType::RParen)) {
        error("expected a closing parenthesis ) for while block");
        return loop;
    }

    // { statement(s) } with continue & break support
    auto prev_loop_node = current_loop_node;
    current_loop_node = loop;
    auto block = parseBraceBlock("whileloop", loop, allocator);
    if(block.has_value()) {
        auto& blk = block.value();
        loop->body.nodes = std::move(blk.nodes);
        loop->body.set_parent(blk.parent());
    } else {
        error("expected a brace block { statement(s) } when lexing a while block");
        current_loop_node = prev_loop_node;
        return loop;
    }
    current_loop_node = prev_loop_node;

    return loop;

}