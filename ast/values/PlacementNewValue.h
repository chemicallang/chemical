// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/Value.h"

class PlacementNewValue : public Value {
public:

    Value* pointer;
    Value* value;
    SourceLocation location;

    inline PlacementNewValue(Value* pointer, Value* value, SourceLocation location) : pointer(pointer), value(value), location(location) {

    }

    ValueKind val_kind() override {
        return ValueKind::NewValue;
    }

    SourceLocation encoded_location() override {
        return location;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) override;

    llvm::Value* llvm_value(Codegen &gen, BaseType *type = nullptr) override;

#endif

};