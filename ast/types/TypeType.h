// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/BaseType.h"

class TypeType : public BaseType {
public:

    SourceLocation location;

    inline TypeType(SourceLocation location) : location(location) {

    }

    SourceLocation encoded_location() override {
        return location;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    bool satisfies(BaseType *type) override {
        return type->kind() == BaseTypeKind::TypeContainingValue;
    }

    bool is_same(BaseType *type) override {
        return type->kind() == BaseTypeKind::TypeType;
    }

    BaseType* copy(ASTAllocator &allocator) const override {
        return new (allocator.allocate<TypeType>()) TypeType(location);
    }


};