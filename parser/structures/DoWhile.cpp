// Copyright (c) Qinetik 2024.

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

    auto loop = new (allocator.allocate<DoWhileLoop>()) DoWhileLoop(nullptr, { nullptr, 0 }, parent_node, loc_single(tok));

    // { statement(s) } with continue & break support
    auto prev_loop_node = current_loop_node;
    current_loop_node = loop;
    auto block = parseBraceBlock("dowhileloop", loop, allocator);
    if(block.has_value()) {
        auto& blk = block.value();
        loop->body.nodes = std::move(blk.nodes);
        loop->body.parent_node = blk.parent_node;
    } else {
        error("expected a brace block { statement(s) } when lexing a while block");
        current_loop_node = prev_loop_node;
        return loop;
    }
    current_loop_node = prev_loop_node;

    if(!consumeWSOfType(TokenType::WhileKw)) {
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