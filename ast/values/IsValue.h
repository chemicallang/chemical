// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/BoolType.h"

class IsValue : public Value {
public:

    Value* value;
    TypeLoc type;
    bool is_negating;

    /**
     * constructor
     */
    constexpr IsValue(
            Value* value,
            TypeLoc type,
            bool is_negating,
            BoolType* boolType,
            SourceLocation location
    ) : Value(ValueKind::IsValue, boolType, location), value(value), type(type), is_negating(is_negating) {

    }

    BoolType* getType() const noexcept {
        return (BoolType*) Value::getType();
    }

    IsValue *copy(ASTAllocator& allocator) final {
        return new (allocator.allocate<IsValue>()) IsValue(
                value->copy(allocator),
                type.copy(allocator),
                is_negating,
                getType(),
                encoded_location()
        );
    }

    /**
     * std::nullopt means unknown, true or false means it evaluated
     */
    std::optional<bool> get_comp_time_result();

    BaseType* known_type() final {
        return (BaseType*) &BoolType::instance;
    }

    BaseType* create_type(ASTAllocator &allocator) final {
        return new (allocator.allocate<BoolType>()) BoolType();
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};