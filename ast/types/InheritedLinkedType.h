// Copyright (c) Qinetik 2024.

#pragma once

#include "ReferencedType.h"
#include "ast/base/AccessSpecifier.h"

class InheritedLinkedType : public ReferencedType {
public:

    /**
     * by default public is used
     */
    AccessSpecifier specifier = AccessSpecifier::Public;

    /**
     * constructor
     */
    InheritedLinkedType(std::string type, ASTNode* linked, AccessSpecifier specifier) : ReferencedType(std::move(type), linked), specifier(specifier) {

    }

    /**
     * copy the linked type
     */
    [[nodiscard]]
    InheritedLinkedType *copy() const override {
        return new InheritedLinkedType(type, linked, specifier);
    }

};