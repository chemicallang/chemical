// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInstantiationPass.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/If.h"
#include "ast/structures/Scope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/GenericType.h"

void sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope) {
    GenericInstantiationPass visitor(resolver);
    visitor.visit(scope);
}

void GenericInstantiationPass::VisitScope(Scope* node) {
    for (const auto child : node->nodes) {
        visit(child);
    }
}

void GenericInstantiationPass::VisitNamespaceDecl(Namespace* node) {
    for (const auto child : node->nodes) {
        visit(child);
    }
}

void GenericInstantiationPass::VisitIfStmt(IfStatement* node) {
    if (node->computed_scope.has_value()) {
        const auto scope = node->computed_scope.value();
        if (scope) {
            VisitByPtrTypeNoNullCheck(scope);
        }
    }
}

void GenericInstantiationPass::VisitGenericTypeDecl(GenericTypeDecl* node) {
    auto& allocator = *linker.ast_allocator;
    // inline instantiations are stored from link signature
    // finalizing signature of inline instantiations that occurred before link_signature
    for (auto& inst : node->inline_instantiations) {
        GenericTypeDecl::finalize_signature(allocator, inst.first);
    }
    // finalize the signature of all instantiations
    for (auto& inst : node->inline_instantiations) {
        linker.genericInstantiator.FinalizeSignature(node, inst.first, inst.second);
    }
}

void GenericInstantiationPass::VisitGenericType(GenericType* type) {
    RecursiveVisitor<GenericInstantiationPass>::VisitGenericType(type);
    type->instantiate(linker.genericInstantiator, type_location);
}

void GenericInstantiationPass::VisitFunctionDecl(FunctionDeclaration* node) {
    for(auto& param : node->params) {
        visit_it(param);
    }
    visit_it(node->returnType);
}

void GenericInstantiationPass::VisitVarInitStmt(VarInitStatement* node) {
    if(node->type) {
        visit_it(node->type);
    }
}
