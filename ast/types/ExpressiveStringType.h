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

    BaseType* copy(ASTAllocator &allocator) final {
        // why does this return itself (without copying)
        // because the type exists in type builder
        // it is initialized once in the type builder
        // this will never be copied
        return this;
    }

};