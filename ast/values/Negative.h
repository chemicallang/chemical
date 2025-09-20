// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

// A value that's preceded by a negative operator -value
class NegativeValue : public Value {
private:

    Value* value;

public:

    /**
     * constructor
     */
    constexpr NegativeValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NegativeValue, value->getType(), location), value(value) {

    }

    inline Value* getValue() const noexcept {
        return value;
    }

    void setValue(Value* newValue) {
        value = newValue;
        setType(newValue->getType());
    }

    uint64_t byte_size(bool is64Bit) final;

    bool primitive() final;

    Value* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NegativeValue>()) NegativeValue(value->copy(allocator), encoded_location());
    }

    Value* evaluated_value(InterpretScope &scope) final;

#ifdef COMPILER_BUILD

    llvm::Type* llvm_type(Codegen &gen) final {
        return getType()->llvm_type(gen);
    }

    llvm::Value* llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    void determine_type(TypeBuilder& typeBuilder, ASTDiagnoser& diagnoser);

};