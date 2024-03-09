// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/AbstractStringToken.h"
#include "ast/statements/Assignment.h"

lex_ptr<ASTNode> Parser::parseVarAssignStatement() {
    auto chain = parseAccessChain();
    if (!chain.has_value()) {
        return std::nullopt;
    }
    auto operation = consume_op_token();
    if(!consume_op('=') && operation != Operation::Increment && operation != Operation::Decrement) {
        if(operation.has_value()) {
            error("expected a equal operator after the operation token");
        }
        return std::move(chain.value());
    }
    if(!operation.has_value()) {
        operation = Operation::Equal;
    }
    auto value = parseExpression();
    if (value.has_value()) {
        return std::make_unique<AssignStatement>(std::move(chain.value()), std::move(value.value()), operation.value());
    } else {
        if(operation.has_value() && (operation == Operation::Increment) || (operation == Operation::Decrement)) {
            return std::make_unique<AssignStatement>(std::move(chain.value()), std::make_unique<IntValue>(1), operation.value());
        } else {
            error("expected a value after the operation in var assignment");
        }
    }
    return std::nullopt;
}