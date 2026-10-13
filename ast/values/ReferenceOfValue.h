// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"

class ReferenceOfValue : public Value {
public:

    Value* value;
    bool is_mutable = false;
    // When evaluated_value() is called and wraps a FunctionCall result,
    // stores the inner evaluated result (the struct temp) so the enclosing
    // expression can destruct it after use. Set during evaluated_value().
    Value* innerEvaluatedResult = nullptr;

    /**
     * constructor
     */
    constexpr ReferenceOfValue(
        Value* value,
        bool is_mutable,
        ReferenceType* refType,
        SourceLocation location
    ) : Value(ValueKind::ReferenceOfValue, refType, location), value(value), is_mutable(is_mutable) {

    }

    inline ReferenceType* getType() {
        return (ReferenceType*) Value::getType();
    }

    uint64_t byte_size(const TargetData& target) final;

private:
    inline ReferenceOfValue* copy_with(ASTAllocator& allocator, Value* val) {
        return new (allocator.allocate<ReferenceOfValue>()) ReferenceOfValue(
                val,
                is_mutable,
                getType(),
                encoded_location()
        );
    }
public:

    ReferenceOfValue *copy(ASTAllocator& allocator) final {
        return copy_with(allocator, value->copy(allocator));
    }

    void determine_type();

    ASTNode *linked_node() final {
        return value->linked_node();
    }

    Value* child(InterpretScope& scope, const chem::string_view& name) override {
        return value->child(scope, name);
    }

    Value* evaluated_value(InterpretScope& scope) override;


};
