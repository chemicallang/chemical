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
    std::vector<Value*> values;
    CSTToken* token;

    IndexOperator(std::vector<Value*> indexes, CSTToken* token) : values(std::move(indexes)), token(token) {

    }

    CSTToken* cst_token() final {
        return token;
    }

    ValueKind val_kind() final {
        return ValueKind::IndexOperator;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

    ASTNode *linked_node() final;

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type);

    bool link(SymbolResolver &linker, std::vector<ChainValue *> &values, unsigned int index, BaseType *expected_type) final {
        const auto values_size = values.size();
        const auto parent_index = index - 1;
        const auto parent = parent_index < values_size ? values[parent_index] : nullptr;
        if(parent) {
            return find_link_in_parent(parent, linker, expected_type);
        } else {
            return link(linker, (Value*&) values[index], expected_type);
        }
    }

    void relink_parent(ChainValue *parent) final;

    bool primitive() final {
        return false;
    }

    Value *find_in(InterpretScope &scope, Value *parent) final;

    IndexOperator* copy(ASTAllocator& allocator) final;

    BaseType* create_type(ASTAllocator &allocator) final;

//    hybrid_ptr<BaseType> get_base_type() final;

    BaseType* known_type() final;

    [[nodiscard]]
    ValueType value_type() const final;

#ifdef COMPILER_BUILD

    llvm::Value *elem_pointer(Codegen &gen, llvm::Type *, llvm::Value *ptr);

    llvm::Value *elem_pointer(Codegen &gen, ASTNode *arr);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Value *access_chain_pointer(Codegen &gen, std::vector<ChainValue*> &values, std::vector<std::pair<Value *, llvm::Value *>> &destructibles, unsigned int until) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) final;

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<ChainValue*> &chain, unsigned int index) final;

    llvm::FunctionType *llvm_func_type(Codegen &gen) final;

    bool add_member_index(Codegen &gen, Value *parent, std::vector<llvm::Value *> &indexes) final;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) final;

#endif

};