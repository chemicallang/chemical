// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * multiple value
 */
class RawLiteral : public Value {
public:

    /**
     * the value being emitted
     */
    chem::string_view value;

    /**
     * constructor
     */
    constexpr RawLiteral(
        BaseType* type,
        SourceLocation location
    ) : Value(ValueKind::RawLiteral, type, location) {

    }

    /**
     * constructor
     */
    constexpr RawLiteral(
            chem::string_view value,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::RawLiteral, type, location), value(value) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto other = new (allocator.allocate<RawLiteral>()) RawLiteral(
            getType(), encoded_location()
        );
        other->value = value;
        return other;
    }


};