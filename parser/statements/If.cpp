// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "parser/Parser.h"
#include "ast/structures/If.h"

std::optional<std::pair<std::unique_ptr<Value>, Scope>> Parser::parseSingleIfBlock() {
    if(!consume("if")) {
        return std::nullopt;
    }
    if(consume_op('(')) {
        auto condition = parseExpression();
        if(!condition.has_value()) {
            error("expected a conditional expression for 'if' condition");
            return std::nullopt;
        }
        if(!consume_op(')')) {
            error("expected a ending parenthesis ')' in 'if' condition");
            return std::nullopt;
        }
        if(consume_op('{')) {
            auto scope = parseScope();
            if(!consume_op('}')) {
                error("expected a ending brace '}' for 'if' block");
            }
            return std::pair(std::move(condition.value()), std::move(scope));
        } else {
            error("expected a starting brace after the ')' for the 'if' block");
        }
    } else {
        error("expected a starting parenthesis '('");
    }
    return std::nullopt;
}

lex_ptr<IfStatement> Parser::parseIfStatement() {
    auto singleIf = parseSingleIfBlock();
    if(singleIf.has_value()) {
        auto elseIfs = std::vector<std::pair<std::unique_ptr<Value>, Scope>>();
        std::optional<Scope> elseScope = std::nullopt;
        while(consume("else")) {
            if(consume_op('{')) {
                elseScope.emplace(parseScope());
                if(!consume_op('}')) {
                    error("expected a ending brace '}' for 'else' block");
                }
                break;
            } else {
                auto elseIf = parseSingleIfBlock();
                if (elseIf.has_value()) {
                    elseIfs.emplace_back(std::move(elseIf.value()));
                } else {
                    error("expected a else if / else block after 'if' statement");
                    break;
                }
            }
        }
        return std::make_unique<IfStatement>(std::move(singleIf.value().first), std::move(singleIf.value().second), std::move(elseIfs), std::move(elseScope));
    }
    return std::nullopt;
}

bool Parser::parseIfStatementBool() {
    return parse_return_bool([&]() -> lex_ptr<IfStatement> {
        return parseIfStatement();
    });
}