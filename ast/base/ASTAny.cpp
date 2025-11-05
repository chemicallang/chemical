// Copyright (c) Chemical Language Foundation 2025.

#include "ASTAny.h"
#include "Value.h"
#include "ASTNode.h"
#include "BaseType.h"
#include <iostream>
#include "std/except.h"

#ifdef COMPILER_BUILD

llvm::Type *ASTAny::llvm_type(Codegen &gen) {
    CHEM_THROW_RUNTIME("llvm_type called on ASTAny");
    return nullptr;
}

#endif

std::string to_string(AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Private:
            return "private";
        case AccessSpecifier::Public:
            return "public";
        case AccessSpecifier::Protected:
            return "protected";
        case AccessSpecifier::Internal:
            return "internal";
        default:
            return "";
    }
}

std::string ASTAny::representation() {
    switch (any_kind()) {
        case ASTAnyKind::Value:
            return ((Value*) this)->representation();
        case ASTAnyKind::Type:
            return ((BaseType*) this)->representation();
        case ASTAnyKind::Node:
            return ((ASTNode*) this)->representation();
        default:
            return "[UnknownAstAny:representation]";
    }
}

ASTNode* ASTAny::get_ref_linked_node() {
    switch (any_kind()) {
        case ASTAnyKind::Value:
            return ((Value*) this)->linked_node();
        case ASTAnyKind::Type:
            return ((BaseType*) this)->linked_node();
        default:
            return nullptr;
    }
}

ASTAny::~ASTAny() = default;