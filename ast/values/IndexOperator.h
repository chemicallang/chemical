// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include <utility>
#include "ast/base/ChainValue.h"

class IndexOperator : public ChainValue {
public:

    ChainValue *parent_val;
    std::vector<std::unique_ptr<Value>> values;

    explicit IndexOperator(std::vector<std::unique_ptr<Value>> indexes) : values(std::move(indexes)) {

    }

    hybrid_ptr<BaseType> get_base_type() override;

    BaseType* known_type() override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    IndexOperator *as_index_op() override {
        return this;
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    ASTNode *linked_node() override;

    void find_link_in_parent(ChainValue *parent, SymbolResolver &resolver) override;

    void relink_parent(ChainValue *parent) override;

    bool primitive() override {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) override;

    IndexOperator* copy() override;

#ifdef COMPILER_BUILD

    llvm::Value *elem_pointer(Codegen &gen, llvm::Type *, llvm::Value *ptr);

    llvm::Value *elem_pointer(Codegen &gen, ASTNode *arr);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value *access_chain_pointer(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &values, std::vector<std::pair<Value *, llvm::Value *>> &destructibles, unsigned int until) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<ChainValue>> &chain, unsigned int index) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    std::unique_ptr<BaseType> create_type() override;

};