// Copyright (c) Chemical Language Foundation 2025.

#include "GenericInstantiationPass.h"
#include "ast/values/GenericInstIdentifier.h"
#include "compiler/SymbolResolver.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/If.h"
#include "ast/structures/Scope.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/VarInit.h"
#include "ast/types/GenericType.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/AccessChain.h"
#include "ast/utils/GenericUtils.h"
#include "GenericInstantiatorPassAPI.h"

GenInstSignatureResult sym_res_generic_instantiation(SymbolResolver& resolver, Scope* scope, SymResSignatureResult& result, const SymbolRange& range) {
    GenericInstantiationPass visitor(resolver);
    // first we finalize inline instantiations
    // inline instantiations are stored from link signature
    // finalizing signature of inline instantiations that occurred before link_signature
    auto& allocator = *resolver.ast_allocator;
    for (auto& inst : result.inline_instantiations) {
        GenericTypeDecl::finalize_signature(allocator, inst.first);
    }
    // finalize the signature of all instantiations (registration only — this pass visits signatures, not bodies)
    for (auto& inst : result.inline_instantiations) {
        visitor.generic_instantiator.FinalizeSignature(inst.first->generic_parent, inst.first, inst.second);
    }
    // now doing the actual
    visitor.visit(scope);
    return GenInstSignatureResult {
        .has_errors = visitor.diagnoser.has_errors(),
        .diagnostics = std::move(visitor.diagnoser.diagnostics)
    };
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
    // this pass only needs registration — it visits signatures, not bodies
    val->ensure_specialized_container(generic_instantiator, diagnoser, InstantiationRequirement::Registration);
}

void GenericInstantiationPass::VisitGenericType(GenericType* type) {
    RecursiveVisitor<GenericInstantiationPass>::VisitGenericType(type);
    // this pass only needs registration — it visits signatures, not bodies
    type->instantiate(generic_instantiator, type_location, InstantiationRequirement::Registration);
}

void GenericInstantiationPass::VisitFunctionDecl(FunctionDeclaration* node) {
    for(auto& param : node->params) {
        visit_it(param);
    }
    visit_it(node->returnType);
}

void GenericInstantiationPass::VisitGenericInstIdentifier(GenericInstIdentifier* ref) {
    // a bare generic function reference used as a value in a global initializer
    // (`var f : (x : int) => int = ident<int>`) must be registered here, because
    // function body references are handled by SymResLinkBody / the generic
    // instantiator and this pass does not visit function bodies. the instantiated
    // body is finalized once the generic declaration's body is linked.
    RecursiveVisitor<GenericInstantiationPass>::VisitGenericInstIdentifier(ref);
    const auto value = ref->getIdentifier();
    const auto linked = value->linked;
    if(linked == nullptr || linked->kind() != ASTNodeKind::GenericFuncDecl) {
        ref->setType(value->getType());
        return;
    }
    const auto gen_decl = linked->as_gen_func_decl_unsafe();
    for(auto& type : ref->generic_list) {
        visit(type);
    }
    std::vector<TypeLoc> generic_args;
    if(!initialize_generic_args(diagnoser, generic_args, gen_decl->generic_params, ref->generic_list)) {
        ref->setType(value->getType());
        return;
    }
    if(!check_inferred_generic_args(diagnoser, generic_args, gen_decl->generic_params, value->encoded_location())) {
        ref->setType(value->getType());
        return;
    }
    for(auto& type : generic_args) {
        if(type) {
            type = { type->canonical(), type.getLocation() };
        }
    }
    const auto concrete = gen_decl->register_generic_args(
        generic_instantiator, generic_args, value->encoded_location(),
        InstantiationRequirement::Registration
    );
    if(concrete != nullptr) {
        value->linked = concrete;
        value->setType(concrete->known_type());
    }
    ref->setType(value->getType());
}

void GenericInstantiationPass::VisitAccessChain(AccessChain* chain) {
    RecursiveVisitor<GenericInstantiationPass>::VisitAccessChain(chain);
    // a chain may end in a generic function reference; VisitGenericInstIdentifier
    // has resolved it, so refresh the chain's type from the leaf
    const auto last = chain->values.back();
    if(last->kind() == ValueKind::GenericInstIdentifier) {
        chain->setType(last->getType());
    }
}