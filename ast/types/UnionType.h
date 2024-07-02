// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/BaseType.h"
#include "ordered_map.h"
#include "ast/structures/BaseDefMember.h"

class VariablesContainer;

class UnionType : public BaseType {
public:

    UnionType() = default;

    virtual VariablesContainer* variables_container() = 0;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseTypeKind kind() const override {
        return BaseTypeKind::Union;
    }

    uint64_t byte_size(bool is64Bit) override;

    bool equals(UnionType *type) const {
        return type->byte_size(true) == const_cast<UnionType*>(this)->byte_size(true);
    }

    bool is_same(BaseType *type) override {
        return kind() == type->kind() && equals(static_cast<UnionType *>(type));
    }

    virtual BaseType *copy() const = 0;

    bool satisfies(ValueType type) override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};