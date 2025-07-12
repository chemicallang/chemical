// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include <memory>
#include "ast/base/Value.h"

// A value that's preceded by a not operator !value
class NotValue : public Value {
public:

    Value* value;

    /**
     * constructor
     */
    constexpr NotValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NotValue, location), value(value) {}

    bool primitive() final;

    bool compile_time_computable() override {
        return value->compile_time_computable();
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

    NotValue* copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<NotValue>()) NotValue(value->copy(allocator), encoded_location());
    }

    BaseType* create_type(ASTAllocator &allocator) final;

    Value* evaluated_value(InterpretScope &scope) override;

    BaseType* known_type() final;

};