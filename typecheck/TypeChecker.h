// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include <memory>
#include <vector>
#include <string>
#include "ast/base/Visitor.h"

class ASTNode;

class TypeChecker : Visitor {
public:

    std::vector<std::string> errors;

    TypeChecker();

    void visit(VarInitStatement *init) override;

    void visit(DoWhileLoop *doWhileLoop) override;

    void visit(ForLoop *forLoop) override;

    void visit(IfStatement *ifStatement) override;

    void visit(Scope *scope) override;

    void type_check(std::vector<std::unique_ptr<ASTNode>>& nodes);

    inline void error(const std::string& err) {
        errors.push_back(err);
    }

};