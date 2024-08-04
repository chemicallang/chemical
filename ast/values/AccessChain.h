// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <memory>
#include "ast/base/ASTNode.h"
#include "ast/base/Value.h"

/**
 * access chain represents a way to access things in programming language for example
 * x.y.z is an access chain, z member of the y of the x is being accessed
 * x.y.z() where z is a function call, z function is assumed to be present in y and y in z
 * z.y.z[0] similarly z is an index operator here
 */
class AccessChain : public ASTNode, public Value {
public:

    std::vector<std::unique_ptr<Value>> values;
    ASTNode* parent_node;
    bool is_node;

    AccessChain(ASTNode* parent_node, bool is_node) : parent_node(parent_node), is_node(is_node) {

    }

    AccessChain(std::vector<std::unique_ptr<Value>> values, ASTNode* parent_node, bool is_node);

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void link(SymbolResolver &linker, std::unique_ptr<Value>& value_ptr) override;

    void link_without_parent();

    void declare_and_link(SymbolResolver &linker) override;

    void accept(Visitor *visitor) override;

    Value *copy() override;

    bool primitive() override;

    bool reference() override;

    void interpret(InterpretScope &scope) override;

    std::unique_ptr<BaseType> create_type() override;

    std::unique_ptr<BaseType> create_value_type() override;

    hybrid_ptr<BaseType> get_base_type() override;

    hybrid_ptr<BaseType> get_value_type() override;

    uint64_t byte_size(bool is64Bit) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::AllocaInst *llvm_allocate(Codegen &gen, const std::string &identifier) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

    unsigned int store_in_struct(
            Codegen &gen,
            StructValue *parent,
            llvm::Value *allocated,
            std::vector<llvm::Value *> idxList,
            unsigned int index
    ) override;

    unsigned int store_in_array(
            Codegen &gen,
            ArrayValue *parent,
            llvm::AllocaInst *ptr,
            std::vector<llvm::Value *> idxList,
            unsigned int index
    ) override;

#endif

    AccessChain *as_access_chain() override {
        return this;
    }

    void evaluate_children(InterpretScope &scope) override;

    Value *parent(InterpretScope &scope);

    inline hybrid_ptr<Value> parent_value(InterpretScope &scope);

    void set_identifier_value(InterpretScope &scope, Value *rawValue, Operation op) override;

    Value *pointer(InterpretScope &scope);

    Value *scope_value(InterpretScope &scope) override;

    hybrid_ptr<Value> evaluated_value(InterpretScope &scope);

    std::unique_ptr<Value> create_evaluated_value(InterpretScope &scope) override;

    inline std::string chain_representation() {
        return Value::representation();
    }

    ASTNode *linked_node() override;

    [[nodiscard]]
    ValueType value_type() const override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

};

Value* get_grandpa_value(std::vector<std::unique_ptr<Value>> &chain_values, unsigned int index);