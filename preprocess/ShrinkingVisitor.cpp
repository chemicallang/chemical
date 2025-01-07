// Copyright (c) Qinetik 2024.

#include "ShrinkingVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/Namespace.h"

void ShrinkingVisitor::visit(std::vector<ASTNode*>& nodes) {
    for(const auto node : nodes) {
        node->accept(this);
    }
}

void shrink(std::optional<Scope>& scope) {
    if(scope.has_value()) {
        scope->nodes.clear();
        scope->nodes.shrink_to_fit();
    }
}

void shrink(FunctionDeclaration* decl) {
    if(decl->is_comptime() || (decl->is_generic() && decl->specifier() != AccessSpecifier::Private)) {
        return;
    }
    shrink(decl->body);
}

void ShrinkingVisitor::visit(FunctionDeclaration *funcDecl) {
    shrink(funcDecl);
}

void ShrinkingVisitor::visit(ExtensionFunction *exFunc) {
    shrink(exFunc);
}

void ShrinkingVisitor::visit(StructDefinition *def) {
    if(def->is_generic() && def->specifier() != AccessSpecifier::Private) {
        return;
    }
    for(auto& func : def->functions()) {
        shrink(func);
    }
}

void ShrinkingVisitor::visit(InterfaceDefinition *def) {
    for(auto& func : def->functions()) {
        shrink(func);
    }
}

void ShrinkingVisitor::visit(ImplDefinition *def) {
    for(auto& func : def->functions()) {
        shrink(func);
    }
}

void ShrinkingVisitor::visit(Namespace *ns) {
    visit(ns->nodes);
}