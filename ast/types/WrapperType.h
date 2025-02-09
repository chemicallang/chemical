// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/base/Value.h"

/**
 * Wrapper type wraps a type, which is evaluated when pure_type is called
 */
class WrapperType : public BaseType {
public:

    /**
     * actual type
     */
    BaseType* actual_type;

    /**
     * constructor
     */
    explicit inline WrapperType(BaseType* actual_type) : actual_type(actual_type) {

    }

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::WrapperType;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(BaseType *type) override {
        return actual_type != nullptr && actual_type->satisfies(type);
    }

    bool is_same(BaseType *type) override {
        return actual_type != nullptr && actual_type->is_same(type);
    }

    SourceLocation encoded_location() override {
        return actual_type ? actual_type->encoded_location() : ZERO_LOC;
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<WrapperType>()) WrapperType(actual_type);
    }

    bool link(SymbolResolver &linker) override {
        return actual_type == nullptr || actual_type->link(linker);
    }

};