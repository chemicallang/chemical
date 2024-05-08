// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/Value.h"

class IndexOperator : public Value {
public:

    IndexOperator(std::vector<std::unique_ptr<Value>> indexes) : values(std::move(indexes)) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    void find_link_in_parent(Value *parent) override;

    bool primitive() override {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) override;

    bool is_direct_value_ref() override;

#ifdef COMPILER_BUILD

    llvm::Value *elem_pointer(Codegen &gen, llvm::Type*, llvm::Value *ptr);

    llvm::Value *elem_pointer(Codegen &gen, ASTNode *arr);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

#endif

    std::unique_ptr<BaseType> create_type() const override;

    std::string representation() const override;

    Value* parent_val;
    std::vector<std::unique_ptr<Value>> values;

};