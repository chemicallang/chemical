// Copyright (c) Qinetik 2024.

#pragma once

#include <utility>
#include "ast/base/BaseType.h"

class GenericType : public BaseType {
public:

    std::unique_ptr<ReferencedType> referenced;
    std::vector<std::unique_ptr<BaseType>> types;
    int16_t generic_iteration = -1;

    GenericType(std::unique_ptr<ReferencedType> referenced);

    GenericType(std::unique_ptr<ReferencedType> referenced, std::vector<std::unique_ptr<BaseType>> types);

    GenericType(std::unique_ptr<ReferencedType> referenced, int16_t generic_itr);

    GenericType(std::string base);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link(SymbolResolver &linker, std::unique_ptr<BaseType>& current) override;

    void report_generic_usage();

    int16_t get_generic_iteration() override {
        return generic_iteration;
    }

    ASTNode *linked_node() override;

    [[nodiscard]]
    BaseTypeKind kind() const override {
        return BaseTypeKind::Generic;
    }

    [[nodiscard]]
    ValueType value_type() const override;

    bool is_same(BaseType *other) override {
        return other->kind() == kind();
    }

    bool satisfies(ValueType value_type) override;

    [[nodiscard]]
    BaseType* copy() const override;

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_param_type(Codegen &gen) override;

#endif

};