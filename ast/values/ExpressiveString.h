// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/ExpressiveStringType.h"

class ExpressiveString : public Value {
public:

    std::vector<Value*> values;

    // The format spec of each value (`${expr:spec}`), parallel to `values`.
    // An empty view means "no spec" — literal text segments always have none.
    std::vector<chem::string_view> specs;

    ExpressiveString(ExpressiveStringType* type, SourceLocation loc) : Value(ValueKind::ExpressiveString, type, loc) {

    }

    inline ExpressiveStringType* getType() {
        return (ExpressiveStringType*) Value::getType();
    }

    // The spec for `values[index]`, or an empty view when there is none.
    chem::string_view spec_at(size_t index) const {
        if(index >= specs.size()) return chem::string_view();
        return specs[index];
    }

    Value* copy(ASTAllocator &allocator) override {
        const auto n = new (allocator.allocate<ExpressiveString>()) ExpressiveString(getType(), encoded_location());
        for(const auto val : values) {
            n->values.emplace_back(val->copy(allocator));
        }
        n->specs = specs;
        return n;
    }

};