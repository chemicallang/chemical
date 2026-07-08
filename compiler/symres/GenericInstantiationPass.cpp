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
    // finalizing signature of instantiations that occurred before link_signature
    for (const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the signature of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // finalizing signature of inline instantiations that occurred before link_signature
    for (auto& inst : node->inline_instantiations) {
        node->finalize_signature(allocator, inst.first);
    }
    // finalize the signature of all instantiations
    for (auto& inst : node->inline_instantiations) {
        linker.genericInstantiator.FinalizeSignature(node, inst.first, inst.second);
    }
}

void GenericInstantiationPass::VisitGenericFuncDecl(GenericFuncDecl* node) {
    // finalizing the signature of every function that was instantiated before link_signature
    auto& allocator = *linker.ast_allocator;
    for (const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    auto resolved_sig = node->master_impl->data.signature_resolved;
    // since these instantiations were created before link signature
    // we must set the resolved_signature to true, which is false before link signature
    for (const auto inst : node->instantiations) {
        inst->data.signature_resolved = resolved_sig;
    }
    // finalize the signatures of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
}

void GenericInstantiationPass::VisitGenericStructDecl(GenericStructDecl* node) {
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for (const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the signature of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for (const auto inst : node->instantiations) {
        for (const auto func : inst->master_functions()) {
            func->FunctionType::data.signature_resolved = true;
        }
    }
    // now we must generate functions, this is required
    // because some generic members can be destructible, in that case
    // a destructor must be generated
    for (const auto inst : node->instantiations) {
        inst->generate_functions(*linker.ast_allocator, linker, inst);
    }
    // we must generate functions for master as well
    // because user can call the constructor of master implementation, which should be available
    // if this creates a destructor, then it would be copied in instantiations and instantiations won't generate another destructor
    // similarly for default constructor
    node->master_impl->generate_functions(*linker.ast_allocator, linker, node);
}

void GenericInstantiationPass::VisitGenericUnionDecl(GenericUnionDecl* node) {
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for (const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the signature of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for (const auto inst : node->instantiations) {
        for (const auto func : inst->master_functions()) {
            func->FunctionType::data.signature_resolved = true;
        }
    }
}

void GenericInstantiationPass::VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for (const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the signature of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for (const auto inst : node->instantiations) {
        for (const auto func : inst->master_functions()) {
            func->FunctionType::data.signature_resolved = true;
        }
    }
}

void GenericInstantiationPass::VisitGenericVariantDecl(GenericVariantDecl* node) {
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for (const auto inst : node->instantiations) {
        node->finalize_signature(allocator, inst);
    }
    // finalize the signature of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for (const auto inst : node->instantiations) {
        for (const auto func : inst->master_functions()) {
            func->FunctionType::data.signature_resolved = true;
        }
    }
    // now we must generate functions, this is required
    // because some generic members can be destructible, in that case
    // a destructor must be generated
    for (const auto inst : node->instantiations) {
        inst->generate_functions(*linker.ast_allocator, linker, inst);
    }
    // we must generate functions for master as well
    // because user can call the constructor of master implementation, which should be available
    // if this creates a destructor, then it would be copied in instantiations and instantiations won't generate another destructor
    // similarly for default constructor
    node->master_impl->generate_functions(*linker.ast_allocator, linker, node);
}

void GenericInstantiationPass::VisitGenericImplDecl(GenericImplDecl* node) {
    // finalizing signature of instantiations that occurred before link_signature
    auto& allocator = *linker.ast_allocator;
    for (const auto inst : node->instantiations) {
        GenericImplDecl::finalize_signature(allocator, inst);
    }
    // finalize the signature of all instantiations
    linker.genericInstantiator.FinalizeSignature(node, node->instantiations);
    // since these instantiations were created before link_signature
    // the functions have signature_resolved set to false, we must fix that
    for (const auto inst : node->instantiations) {
        for (const auto func : inst->master_functions()) {
            func->FunctionType::data.signature_resolved = true;
        }
    }
}

void GenericInstantiationPass::VisitGenericType(GenericType* type) {
    RecursiveVisitor<GenericInstantiationPass>::VisitGenericType(type);
    if(linker.generic_context) {
        type->instantiate_inline(linker.genericInstantiator, type_location);
    } else {
        type->instantiate(linker.genericInstantiator, type_location);
    }
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
