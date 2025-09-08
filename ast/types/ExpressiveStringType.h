// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/BaseType.h"

class ExpressiveStringType : public BaseType {
public:

    ExpressiveStringType() : BaseType(BaseTypeKind::ExpressiveString) {

    }

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::ExpressiveString;
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<ExpressiveStringType>()) ExpressiveStringType();
    }

};