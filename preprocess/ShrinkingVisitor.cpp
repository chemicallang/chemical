// Copyright (c) Qinetik 2024.

#include "ShrinkingVisitor.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/InterfaceDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/ExtensionFunction.h"

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

void ShrinkingVisitor::visit(FunctionDeclaration *funcDecl) {
    shrink(funcDecl->body);
}

void ShrinkingVisitor::visit(ExtensionFunction *exFunc) {
    shrink(exFunc->body);
}

void ShrinkingVisitor::visit(StructDefinition *def) {
    for(auto& func : def->functions) {
        shrink(func.second->body);
    }
}

void ShrinkingVisitor::visit(InterfaceDefinition *def) {
    for(auto& func : def->functions) {
        shrink(func.second->body);
    }
}

void ShrinkingVisitor::visit(ImplDefinition *def) {
    for(auto& func : def->functions) {
        shrink(func.second->body);
    }
}