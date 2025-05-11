// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/DoWhileLoop.h"

DoWhileLoop* Parser::parseDoWhileLoop(ASTAllocator& allocator) {

    auto& tok = *token;

    if(tok.type != TokenType::DoKw) {
        return nullptr;
    }

    token++;

    auto loop = new (allocator.allocate<DoWhileLoop>()) DoWhileLoop(nullptr, parent_node, loc_single(tok));

    // { statement(s) } with continue & break support
    auto block = parseBraceBlock("dowhileloop", loop, allocator);
    if(block.has_value()) {
        auto& blk = block.value();
        loop->body.nodes = std::move(blk.nodes);
        loop->body.set_parent(blk.parent());
    } else {
        error("expected a brace block { statement(s) } when lexing a while block");
        return loop;
    }

    if(!consumeToken(TokenType::WhileKw)) {
        error("expected 'while' with condition in a do while loop");
        return loop;
    }

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

    return loop;

}