// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInstantiationPass.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/If.h"
#include "ast/structures/Scope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/GenericType.h"
#include "GenericInstantiatorPassAPI.h"

void sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope, SymResSignatureResult& result) {
    GenericInstantiationPass visitor(resolver);
    // first we finalize inline instantiations
    // inline instantiations are stored from link signature
    // finalizing signature of inline instantiations that occurred before link_signature
    auto& allocator = *resolver.ast_allocator;
    for (auto& inst : result.inline_instantiations) {
        GenericTypeDecl::finalize_signature(allocator, inst.first);
    }
    // finalize the signature of all instantiations
    for (auto& inst : result.inline_instantiations) {
        visitor.generic_instantiator.FinalizeSignature(inst.first->generic_parent, inst.first, inst.second);
    }
    // now doing the actual
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

void GenericInstantiationPass::VisitStructValue(StructValue *val) {
    RecursiveVisitor<GenericInstantiationPass>::VisitStructValue(val);
    val->ensure_specialized_container(generic_instantiator, diagnoser);
}

void GenericInstantiationPass::VisitGenericType(GenericType* type) {
    RecursiveVisitor<GenericInstantiationPass>::VisitGenericType(type);
    type->instantiate(generic_instantiator, type_location);
}

void GenericInstantiationPass::VisitFunctionDecl(FunctionDeclaration* node) {
    for(auto& param : node->params) {
        visit_it(param);
    }
    visit_it(node->returnType);
}