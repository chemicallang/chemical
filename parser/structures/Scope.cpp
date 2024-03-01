// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"

void Parser::parseMultipleStatements() {
    do {
        if (parseVariableInitStatement() ||
            parseIfStatementBool() ||
            parseVarAssignStatement()) {
            consume_op(';');
        } else {
            break;
        }
    } while (errors.empty() && (position < tokens.size()));
}

Scope Parser::parseScope() {
    auto prevNodes = std::move(nodes);
    nodes = std::vector<std::unique_ptr<ASTNode>>();
    parseMultipleStatements();
    auto parsedNodes = std::move(nodes);
    parsedNodes.shrink_to_fit();
    nodes = std::move(prevNodes);
    return std::move(Scope(std::move(parsedNodes)));
}