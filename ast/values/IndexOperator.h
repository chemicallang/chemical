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

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    IndexOperator *as_index_op() override {
        return this;
    }

    void link(SymbolResolver &linker) override;

    ASTNode *linked_node() override;

    void find_link_in_parent(Value *parent, SymbolResolver &resolver) override;

    bool primitive() override {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) override;

#ifdef COMPILER_BUILD

    llvm::Value *elem_pointer(Codegen &gen, llvm::Type *, llvm::Value *ptr);

    llvm::Value *elem_pointer(Codegen &gen, ASTNode *arr);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *access_chain_pointer(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, std::vector<std::pair<Value *, llvm::Value *>> &destructibles, unsigned int until) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    std::unique_ptr<BaseType> create_type() const override;

    Value *parent_val;
    std::vector<std::unique_ptr<Value>> values;

};