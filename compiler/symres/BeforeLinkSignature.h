// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "preprocess/visitors/NonRecursiveVisitor.h"
#include "compiler/symres/LinkSignature.h"
#include "ast/structures/Namespace.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/GenericTypeDecl.h"

/**
 * everything that is done before link signature
 */
class BeforeLinkSignature : public NonRecursiveVisitor<BeforeLinkSignature> {
public:

    TopLevelLinkSignature& signatureLinker;

    /**
     * constructor
     */
    constexpr BeforeLinkSignature(TopLevelLinkSignature& linker) : signatureLinker(linker) {

    }

    void link(std::vector<GenericTypeParameter*>& params) {
        for(const auto param : params) {
            if(param->at_least_type) {
                signatureLinker.visit(param->at_least_type);
            }
            if(param->def_type) {
                signatureLinker.visit(param->def_type);
            }
        }
    }

    void VisitNamespaceDecl(Namespace* node);

    inline void VisitGenericFuncDecl(GenericFuncDecl* node) {
        link(node->generic_params);
    }

    inline void VisitGenericImplDecl(GenericImplDecl* node) {
        link(node->generic_params);
    }

    inline void VisitGenericInterfaceDecl(GenericInterfaceDecl* node) {
        link(node->generic_params);
    }

    inline void VisitGenericStructDecl(GenericStructDecl* node) {
        link(node->generic_params);
    }

    inline void VisitGenericUnionDecl(GenericUnionDecl* node) {
        link(node->generic_params);
    }

    inline void VisitGenericVariantDecl(GenericVariantDecl* node) {
        link(node->generic_params);
    }

    inline void VisitGenericTypeDecl(GenericTypeDecl* node) {
        link(node->generic_params);
    }

    void VisitIfStmt(IfStatement* node) {
        // calculated by declare top level
        if(node->computed_scope.has_value()) {
            VisitByPtrTypeNoNullCheck(node->computed_scope.value());
        }
    }

};