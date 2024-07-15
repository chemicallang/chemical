// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/ChainValue.h"
#include "ast/base/ASTNode.h"

class ASTDiagnoser;

class FunctionCall : public ChainValue {
public:

    Value* parent_val;
    std::vector<std::unique_ptr<BaseType>> generic_list;
    std::vector<std::unique_ptr<Value>> values;
    int16_t generic_iteration = 0;

    explicit FunctionCall(
            std::vector<std::unique_ptr<Value>> values
    );

    FunctionCall(FunctionCall &&other) = delete;

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link_values(SymbolResolver &linker);

    void link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    /**
     * provides the base function type to which call is being made
     */
    std::unique_ptr<FunctionType> create_function_type();

    /**
     * provides the base function type to which call is being made
     */
    hybrid_ptr<FunctionType> get_function_type();

    FunctionCall *as_func_call() override;

    ASTNode *linked_node() override;

    void relink_multi_func(ASTDiagnoser* diagnoser);

    void find_link_in_parent(Value *parent, ASTDiagnoser* diagnoser, bool relink_multi);

    void find_link_in_parent(Value *parent, ASTDiagnoser* diagnoser) override {
        find_link_in_parent(parent, diagnoser, true);
    }

    void find_link_in_parent(Value *parent, SymbolResolver &resolver) override;

    bool primitive() override;

    Value *find_in(InterpretScope &scope, Value *parent) override;

    Value *scope_value(InterpretScope &scope) override;

    hybrid_ptr<Value> evaluated_value(InterpretScope &scope) override;

    std::unique_ptr<Value> create_evaluated_value(InterpretScope &scope) override;

    hybrid_ptr<Value> evaluated_chain_value(InterpretScope &scope, Value* parent) override;

    void evaluate_children(InterpretScope &scope) override;

    Value *copy() override;

    void interpret(InterpretScope &scope) override;

    /**
     * this returns the return type of the function
     */
    std::unique_ptr<BaseType> create_type() override;

    /**
     * this returns the return type of the function
     */
    hybrid_ptr<BaseType> get_base_type() override;

    BaseTypeKind type_kind() const override;

    ValueType value_type() const override;

    /**
     * will set the current generic iteration on function declaration
     * returning the previous generic iteration, so you can restore it
     * previous iteration is equal to -2, if couldn't set because it's
     * not a generic function
     */
    int16_t set_curr_itr_on_decl();

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int index) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, std::vector<llvm::Value*>& args);

    llvm::Value *llvm_value(Codegen &gen) override;

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    llvm::InvokeInst *llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind);

    llvm::Value * llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_chain_value(
            Codegen &gen,
            std::vector<llvm::Value*>& args,
            std::vector<std::unique_ptr<Value>>& chain,
            unsigned int until,
            llvm::Value* returnedStruct = nullptr
    );

    llvm::Value * access_chain_value(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned until) override;

    llvm::AllocaInst * access_chain_allocate(Codegen &gen, std::vector<std::unique_ptr<Value>> &values, unsigned int until) override;

    bool add_child_index(Codegen &gen, std::vector<llvm::Value *> &indexes, const std::string &name) override;

#endif

    [[nodiscard]] inline ASTNode* linked() const {
        return parent_val->linked_node();
    }

    /**
     * get linked node as a function
     * you should call this when you are sure, that this call is to a function
     * which is a function declaration
     */
    [[nodiscard]] inline FunctionDeclaration* linked_func() const {
        return linked()->as_function();
    }

    /**
     * if this call refers to a function declaration, returns it, otherwise not
     * so its safe
     */
    [[nodiscard]] inline FunctionDeclaration* safe_linked_func() const {
        return linked() ? linked()->as_function() : nullptr;
    }

};