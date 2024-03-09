// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//
#include "parser/Parser.h"


/**
 * Parse a single while loop
 * @return
 */
lex_ptr<WhileLoop> Parser::parseWhileLoop() {
    if (!consume("while")) {
        return std::nullopt;
    }
    if (consume_op('(')) {
        auto condition = parseExpression();
        if (!condition.has_value()) {
            error("expected a conditional expression for 'while' loop condition");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending brace ')' for the 'while' loop");
            return std::nullopt;
        }
        if (consume_op('{')) {

            auto loop = std::make_unique<WhileLoop>(std::move(condition.value()));

            auto prev_loop_node = current_loop_node;
            // warning : the address of the local variable may escape the function is invalid because we move the loop at the end of the scope
            current_loop_node = loop.get();
            auto prevParseBreak = isParseBreakStatement;
            auto prevParseContinue = isParseContinueStatement;
            isParseBreakStatement = true;
            isParseContinueStatement = true;
            auto scope_nodes = parseScopeNodes();
            isParseBreakStatement = prevParseBreak;
            isParseContinueStatement = prevParseContinue;
            current_loop_node = prev_loop_node;

            loop->body.nodes = std::move(scope_nodes);

            if (!consume_op('}')) {
                error("expected a ending brace '}' for 'while' loop");
                return std::nullopt;
            }

            return loop;

        } else {
            error("expected a starting brace '{' after the ')' for the 'while' loop");
            return std::nullopt;
        }
    } else {
        error("expected a starting parenthesis '('");
        return std::nullopt;
    }
}