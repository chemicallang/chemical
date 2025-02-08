// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"

/**
 * ValueContainingType is a value that contains a type, It is just used to pass
 * values as types, for example a comptime function can return a type by wrapping it in
 * this value
 */
class ValueContainingType : public Value {
public:

    /**
     * the actual value
     */
    BaseType* type;

    /**
     * constructor
     */
    inline ValueContainingType(BaseType* type) : type(type) {

    }

    SourceLocation encoded_location() override {
        return type->encoded_location();
    }

    ValueKind val_kind() override {
        return ValueKind::ValueContainingType;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override {
        return type->link(linker);
    }

};