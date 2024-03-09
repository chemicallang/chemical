// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"

/**
 * Parse a single do while loop
 * @return
 */
lex_ptr<DoWhileLoop> Parser::parseDoWhileLoop() {
    if (!consume("do")) {
        return std::nullopt;
    }
    if (consume_op('{')) {

        auto loop = std::make_unique<DoWhileLoop>();

        auto prevLoopNode = current_loop_node;
        // warning the address the local variable may escape this function, is invalid
        // because we are moving loop at the end of the scope
        current_loop_node = loop.get();
        auto prevParseBreak = isParseBreakStatement;
        auto prevParseContinue = isParseContinueStatement;
        isParseBreakStatement = true;
        isParseContinueStatement = true;
        auto scope_nodes = parseScopeNodes();
        isParseBreakStatement = prevParseBreak;
        isParseContinueStatement = prevParseContinue;
        current_loop_node = prevLoopNode;

        if (!consume_op('}')) {
            error("expected a ending brace '}' for 'while' loop");
            return std::nullopt;
        }
        if(!consume("while")) {
            error("expected a 'while' in 'do while' loop for the condition");
            return std::nullopt;
        }
        if(!consume_op('(')) {
            error("expected a starting brace '(' for the 'do while' loop");
            return std::nullopt;
        }
        auto condition = parseExpression();
        if (!condition.has_value()) {
            error("expected a conditional expression for 'while' loop condition");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending brace ')' for the 'while' loop");
            return std::nullopt;
        }

        loop->body.nodes = std::move(scope_nodes);
        loop->condition = std::move(condition.value());

        return loop;
    } else {
        error("expected a starting brace '{' for the 'do while' loop");
        return std::nullopt;
    }
}