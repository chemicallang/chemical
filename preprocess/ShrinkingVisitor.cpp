// Copyright (c) Qinetik 2024.

#include "ShrinkingVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/Namespace.h"

void ShrinkingVisitor::visit(std::vector<std::unique_ptr<ASTNode>>& nodes) {
    for(auto& node : nodes) {
        node->accept(this);
    }
}

void shrink(std::optional<LoopScope>& scope) {
    if(scope.has_value()) {
        scope->nodes.clear();
        scope->nodes.shrink_to_fit();
    }
}

void shrink(FunctionDeclaration* decl) {
    if(decl->has_annotation(AnnotationKind::CompTime)) {
        return;
    }
    shrink(decl->body);
}

void ShrinkingVisitor::visit(FunctionDeclaration *funcDecl) {
    if(funcDecl->is_generic() && funcDecl->specifier != AccessSpecifier::Private) {
        return;
    }
    shrink(funcDecl);
}

void ShrinkingVisitor::visit(ExtensionFunction *exFunc) {
    if(exFunc->is_generic() && exFunc->specifier != AccessSpecifier::Private) {
        return;
    }
    shrink(exFunc);
}

void ShrinkingVisitor::visit(StructDefinition *def) {
    if(def->is_generic() && def->specifier != AccessSpecifier::Private) {
        return;
    }
    for(auto& func : def->functions()) {
        shrink(func.get());
    }
}

void ShrinkingVisitor::visit(InterfaceDefinition *def) {
    for(auto& func : def->functions()) {
        shrink(func.get());
    }
}

void ShrinkingVisitor::visit(ImplDefinition *def) {
    for(auto& func : def->functions()) {
        shrink(func.get());
    }
}

void ShrinkingVisitor::visit(Namespace *ns) {
    for(auto& node : ns->nodes) {
        node->accept(this);
    }
}