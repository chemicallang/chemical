// Copyright (c) Qinetik 2024.

#include "ASTAny.h"
#include "Value.h"
#include "ASTNode.h"
#include "BaseType.h"
#include <iostream>

#ifdef COMPILER_BUILD

llvm::Type *ASTAny::llvm_type(Codegen &gen) {
    throw std::runtime_error("llvm_type called on ASTAny");
}

#endif

void ASTAny::interpret(InterpretScope &scope) {
#ifdef DEBUG
    throw std::runtime_error("ASTAny::interpret called on a base node");
#endif
}

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