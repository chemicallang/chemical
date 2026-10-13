// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/types/DynamicType.h"

/**
 * the dynamic value is created when you write dyn<Type>(expr)
 */
class DynamicValue : public Value {
public:

    Value* value;

    /**
     * constructor
     */
    DynamicValue(
            Value* value,
            SourceLocation location,
            DynamicType* dynType
    ) : Value(ValueKind::DynamicValue, dynType, location), value(value) {

    }

    DynamicType* getType() const noexcept {
        return (DynamicType*) Value::getType();
    }

    [[nodiscard]]
    BaseType* getInterfaceType() const noexcept {
        return getType()->referenced;
    }

    Value* evaluated_value(InterpretScope& scope) override {
        if(!value) return this;
        auto evaluated = value->evaluated_value(scope);
        if(evaluated && evaluated != value) {
            return evaluated;
        }
        return this;
    }

    Value* child(InterpretScope& scope, const chem::string_view& name) override {
        if(!value) return nullptr;
        // Evaluate the inner value to get the actual struct, then delegate child lookup
        auto inner = value->evaluated_value(scope);
        if(!inner || inner == value) {
            inner = this;
        }
        return inner->child(scope, name);
    }

    Value* copy(ASTAllocator &allocator) override {
        const auto dyn_type = new (allocator.allocate<DynamicType>()) DynamicType(
            getType()->referenced->copy(allocator)
        );
        const auto dyn_val = new (allocator.allocate<DynamicValue>()) DynamicValue(
            value->copy(allocator), encoded_location(), dyn_type
        );
        return dyn_val;
    }


};