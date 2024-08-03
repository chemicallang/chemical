// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include "ast/base/BaseType.h"

class GenericType : public BaseType {
public:

    std::unique_ptr<ReferencedType> referenced;
    std::vector<std::unique_ptr<BaseType>> types;

    GenericType(std::unique_ptr<ReferencedType> referenced);

    GenericType(std::string base);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    ASTNode * linked_node() override;

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Generic;
    }

    bool is_same(BaseType *other) override {
        return other->kind() == kind();
    }

    bool satisfies(ValueType value_type) override;

    [[nodiscard]]
    BaseType* copy() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

#endif

};