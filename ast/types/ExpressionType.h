// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"
#include "ast/utils/Operation.h"

class ExpressionType : public BaseType {
public:

    BaseType* firstType;
    BaseType* secondType;
    bool isLogicalAnd;

    /**
     * contructor
     */
    constexpr ExpressionType(
        BaseType* firstType,
        BaseType* secondType,
        bool isLogicalAnd
    ) : BaseType(BaseTypeKind::ExpressionType), firstType(firstType), secondType(secondType), isLogicalAnd(isLogicalAnd) {
        // do nothing
    }

    uint64_t byte_size(bool is64Bit) override {
        return 0;
    }

    bool satisfies(BaseType *type) override;

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::ExpressionType &&
            isLogicalAnd == type->as_expr_type_unsafe()->isLogicalAnd &&
            firstType->is_same(type->as_expr_type_unsafe()->firstType) &&
            secondType->is_same(type->as_expr_type_unsafe()->secondType);
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<ExpressionType>()) ExpressionType(
                firstType->copy(allocator),
                secondType->copy(allocator),
                isLogicalAnd
        );
    }





};