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

std::string ASTAny::representation() {
    switch (any_kind()) {
        case ASTAnyKind::Value:
            return as_value_unsafe()->representation();
        case ASTAnyKind::Type:
            return as_type_unsafe()->representation();
        case ASTAnyKind::Node:
            return as_node_unsafe()->representation();
    }
}

ASTAny::~ASTAny() = default;