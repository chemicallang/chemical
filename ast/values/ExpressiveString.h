// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/ExpressiveStringType.h"

class ExpressiveString : public Value {
public:

    std::vector<Value*> values;

    ExpressiveString(ExpressiveStringType* type, SourceLocation loc) : Value(ValueKind::ExpressiveString, type, loc) {

    }

    inline ExpressiveStringType* getType() {
        return (ExpressiveStringType*) Value::getType();
    }

    Value* copy(ASTAllocator &allocator) override {
        const auto n = new (allocator.allocate<ExpressiveString>()) ExpressiveString(getType(), encoded_location());
        for(const auto val : values) {
            n->values.emplace_back(val->copy(allocator));
        }
        return n;
    }

};