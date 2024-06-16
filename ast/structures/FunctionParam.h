// Copyright (c) Qinetik 2024.

#pragma once

#include <optional>
#include "ast/base/ASTNode.h"

class BaseFunctionType;

class FunctionParam : public ASTNode {
public:

    FunctionParam(
            std::string name,
            std::unique_ptr<BaseType> type,
            unsigned int index,
            std::optional<std::unique_ptr<Value>> defValue,
            BaseFunctionType* func_type = nullptr
      );

    std::unique_ptr<BaseType> create_value_type() override;

    void accept(Visitor *visitor) override;

    FunctionParam *as_func_param() override {
        return this;
    }

    Value *holding_value() override {
        return defValue.has_value() ? defValue.value().get() : nullptr;
    }

    BaseType *holding_value_type() override {
        return type.get();
    }

#ifdef COMPILER_BUILD

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::Type *llvm_elem_type(Codegen &gen) override;

    llvm::Value *llvm_load(Codegen &gen) override;

    llvm::Value *llvm_ret_load(Codegen &gen, ReturnStatement *returnStmt) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    FunctionParam *copy() const;

    std::string representation() const override;

    ASTNode *child(const std::string &name) override;

    void declare_and_link(SymbolResolver &linker) override;

    ValueType value_type() const override;

    BaseTypeKind type_kind() const override;

    unsigned int index;
    std::string name;
    std::unique_ptr<BaseType> type;
    std::optional<std::unique_ptr<Value>> defValue;
    BaseFunctionType* func_type;

};