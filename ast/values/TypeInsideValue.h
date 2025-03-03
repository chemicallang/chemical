// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/base/BaseType.h"

/**
 * the reason this class exists is because user can write
 * T is int <-- here T is a value, however during generic instantiation we need
 * to replace T with an appropriate type, but Type* is not a Value*, so what we do is
 * during symbol resolution convert T (from variable identifier) to TypeInsideValue
 * where TypeInsideValue contains a linked type, then we replace this type pointer during
 * generic instantiation
 */
class TypeInsideValue : public Value {
public:

    BaseType* type;

    explicit TypeInsideValue(BaseType* type) : Value(ValueKind::TypeInsideValue, type->encoded_location()), type(type) {

    }

    TypeInsideValue* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<TypeInsideValue>()) TypeInsideValue(
            type->copy(allocator)
        );
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) final {
        return type->link(linker);
    }

    BaseType* known_type() override {
        return type;
    }

    BaseType* create_type(ASTAllocator &allocator) override {
        return type->copy(allocator);
    }

};