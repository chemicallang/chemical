// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include "ast/base/ASTNode.h"

class FunctionParam : public ASTNode {
public:

    FunctionParam(
            std::string name,
            std::unique_ptr<BaseType> type,
            unsigned int index,
            bool isVariadic,
            std::optional<std::unique_ptr<Value>> defValue
    );

    std::unique_ptr<BaseType> create_value_type() override;

    void accept(Visitor &visitor) override;

    FunctionParam *as_parameter() override;

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

#endif

    FunctionParam *copy() const;

    std::string representation() const override;

    ASTNode *child(const std::string &name) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, unsigned int index) override;

    void declare_and_link(SymbolResolver &linker) override;

    void undeclare_on_scope_end(SymbolResolver &linker) override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

    unsigned int index;
    std::string name;
    std::unique_ptr<BaseType> type;
    bool isVariadic;
    std::optional<std::unique_ptr<Value>> defValue;
};