// Copyright (c) Qinetik 2024.

#include "TypeChecker.h"
#include "ast/base/ASTNode.h"

TypeChecker::TypeChecker(std::vector<std::unique_ptr<ASTNode>> nodes) : nodes(std::move(nodes)) {

}

void TypeChecker::type_check() {

    for (const auto &node: nodes) {
        node->type_check(*this);
    }

}