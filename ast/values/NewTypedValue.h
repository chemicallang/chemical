// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/ASTNode.h"

/**
 * just responsible for allocating a type on the heap
 */
class NewTypedValue : public Value {
public:

    BaseType* type;
    SourceLocation location;

    inline NewTypedValue(BaseType* type, SourceLocation location) : type(type), location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    ValueKind val_kind() override {
        return ValueKind::NewTypedValue;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

#endif

};