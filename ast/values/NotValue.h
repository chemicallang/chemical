// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a not operator !value
class NotValue : public Value {
private:

    Value* value;

public:

    /**
     * constructor
     */
    constexpr NotValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NotValue, value->getType(), location), value(value) {}

    /**
     * constructor
     */
    constexpr NotValue(
            Value* value,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::NotValue, type, location), value(value) {}

    Value* getValue() const noexcept {
        return value;
    }

    bool primitive() final;

    bool compile_time_computable() override {
        return value->compile_time_computable();
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    NotValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NotValue>()) NotValue(value->copy(allocator), getType(), encoded_location());
    }

    Value* evaluated_value(InterpretScope &scope) override;

    BaseType* known_type() final;

};