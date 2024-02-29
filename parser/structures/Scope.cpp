// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#include "parser/Parser.h"

void Parser::parseMultipleStatements() {
    do {
        parseVariableInitStatement() || parseVarAssignStatement();
    } while (consume_op(';') && !parseError.has_value() && (position < tokens.size()));
}

std::optional<Scope> Parser::parseScope() {
    auto prevNodes = std::move(nodes);
    nodes = std::vector<std::unique_ptr<ASTNode>>();
    parseMultipleStatements();
    auto parsedNodes = std::move(nodes);
    parsedNodes.shrink_to_fit();
    nodes = std::move(prevNodes);
    if (parsedNodes.empty()) {
        return std::nullopt;
    } else {
        return Scope(std::move(parsedNodes));
    }
}