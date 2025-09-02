// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/BoolType.h"

class InValue : public Value {
public:

    bool is_negating;

    Value* value;

    std::vector<Value*> values;


    /**
     * constructor
     */
    constexpr InValue(
            Value* value,
            bool is_negating,
            BoolType* boolType,
            SourceLocation location
    ) : Value(ValueKind::InValue, boolType, location), is_negating(is_negating), value(value) {

    }

    bool primitive() override {
        return false;
    }

    BoolType* getType() const noexcept {
        return (BoolType*) Value::getType();
    }

    InValue *copy(ASTAllocator& allocator) final {
        const auto inValue = new (allocator.allocate<InValue>()) InValue(
                value->copy(allocator),
                is_negating,
                getType(),
                encoded_location()
        );
        for(const auto otherValue : values) {
            inValue->values.emplace_back(otherValue->copy(allocator));
        }
        return inValue;
    }

    BaseType* known_type() final {
        return (BaseType*) &BoolType::instance;
    }

    Value* evaluated_value(InterpretScope &scope) override;

#ifdef COMPILER_BUILD

    void llvm_conditional_branch(Codegen &gen, llvm::BasicBlock *then_block, llvm::BasicBlock *otherwise_block) override;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

#endif

};