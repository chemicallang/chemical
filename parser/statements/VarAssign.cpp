// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/AbstractStringToken.h"
#include "ast/statements/Assignment.h"

bool Parser::parseVarAssignStatement() {
    auto chain = parseAccessChain();
    if (!chain.has_value()) {
        return false;
    }
    auto operation = consume_op_token();
    if(!consume_op('=')) {
        if(operation.has_value()) {
            error("expected a equal operator after the operation token");
        }
        nodes.emplace_back(std::move(chain.value()));
        return true;
    }
    if(!operation.has_value()) {
        operation = Operation::Equal;
    }
    auto value = parseExpression();
    if (value.has_value()) {
        nodes.emplace_back(std::make_unique<AssignStatement>(std::move(chain.value()), std::move(value.value()), operation.value()));
    } else {
        error("expected a value after the operation in var assignment");
    }
    return true;
};