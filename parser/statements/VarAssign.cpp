// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/AbstractStringToken.h"
#include "ast/statements/Assignment.h"

bool Parser::parseVarAssignStatement() {
    auto chain = parseAccessChain();
    if(!chain.has_value()) {
        return false;
    }
    if(consume_op('=')) {
        auto value = parseValueNode();
        if(value.has_value()) {
            nodes.emplace_back(std::make_unique<AssignStatement>(std::move(chain.value()), std::move(value.value())));
        } else {
            error("expected a value after the equal in var assignment");
        }
    } else {
        nodes.emplace_back(std::move(chain.value()));
    }
    return true;
};