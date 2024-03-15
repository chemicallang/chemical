// Copyright (c) Qinetik 2024.

#include "TypeChecker.h"
#include "ast/base/ASTNode.h"
#include "ast/statements/VarInit.h"
#include "ast/structures/DoWhileLoop.h"
#include "ast/base/Value.h"
#include "ast/structures/ForLoop.h"
#include "ast/structures/If.h"
#include "ast/structures/Scope.h"

TypeChecker::TypeChecker() {

}

void TypeChecker::type_check(std::vector<std::unique_ptr<ASTNode>> &nodes) {
    for (const auto &node: nodes) {
        node->accept(*this);
    }
}

void TypeChecker::visit(VarInitStatement *init) {
    if((init->value.has_value() && init->type.has_value()) && !init->type.value()->satisfies(init->value.value()->value_type())) {
        error("var initialization statement type, value doesn't confirm to type");
    }
}

void TypeChecker::visit(DoWhileLoop *doWhileLoop) {
    visit(&doWhileLoop->body);
}

void TypeChecker::visit(ForLoop *forLoop) {
    visit(forLoop->initializer.get());
    forLoop->incrementerExpr->accept(*this);
    visit(&forLoop->body);
}

void TypeChecker::visit(IfStatement *ifStatement) {
    visit(&ifStatement->ifBody);
    for(const auto& eif : ifStatement->elseIfs) {
        visit(const_cast<Scope*>(&eif.second));
    }
    if(ifStatement->elseBody.has_value()) {
        visit(&ifStatement->elseBody.value());
    }
}

void TypeChecker::visit(Scope *scope) {
    for(const auto& node : scope->nodes) {
        node->accept(*this);
    }
}