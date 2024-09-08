// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/ChainValue.h"
#include "ast/base/ASTNode.h"
#include "TypeLinkedValue.h"

class ASTDiagnoser;

class FunctionCall : public ChainValue {
public:

    ChainValue* parent_val;
    std::vector<std::unique_ptr<BaseType>> generic_list;
    std::vector<std::unique_ptr<Value>> values;
    int16_t generic_iteration = 0;
    CSTToken* token;

    FunctionCall(
            std::vector<std::unique_ptr<Value>> values,
            CSTToken* token
    );

    FunctionCall(FunctionCall &&other) = delete;

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::FunctionCall;
    }

    uint64_t byte_size(bool is64Bit) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    void link_values(SymbolResolver &linker);

    /**
     * this should be called only after generic types are known
     */
    void relink_values(SymbolResolver &linker);

    /**
     * if an arg has implicit constructor, we call that implicit constructor instead of
     * just passing that value
     */
    void link_args_implicit_constructor(SymbolResolver &linker);

    void link_gen_args(SymbolResolver &linker);

    bool link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) override;

    /**
     * provides the base function type to which call is being made
     */
    std::unique_ptr<FunctionType> create_function_type();

    /**
     * provides the base function type to which call is being made
     */
    hybrid_ptr<FunctionType> get_function_type();

    /**
     * known type of function call's return
     */
    BaseType* known_type() override;

    /**
     * argument type
     */
    BaseType* get_arg_type(unsigned int index);

    FunctionCall *as_func_call() override {
        return this;
    }

    ASTNode *linked_node() override;

    void relink_multi_func(ASTDiagnoser* diagnoser);

    void link_constructor(SymbolResolver &resolver);

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type, bool link_implicit_constructor);

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver, BaseType *expected_type) override {
        return find_link_in_parent(parent, resolver, expected_type, true);
    }

    bool find_link_in_parent(ChainValue *parent, SymbolResolver &resolver) override {
        return find_link_in_parent(parent, resolver, nullptr, true);
    }

    void relink_parent(ChainValue *parent) override;

    bool primitive() override;

    bool compile_time_computable() override;

    Value *find_in(InterpretScope &scope, Value *parent) override;

    Value *scope_value(InterpretScope &scope) override;

    hybrid_ptr<Value> evaluated_value(InterpretScope &scope) override;

    std::unique_ptr<Value> create_evaluated_value(InterpretScope &scope) override;

    hybrid_ptr<Value> evaluated_chain_value(InterpretScope &scope, Value* parent) override;

    void evaluate_children(InterpretScope &scope) override;

    FunctionCall *copy() override;

    void interpret(InterpretScope &scope) override;

    /**
     * this returns the return type of the function
     */
    std::unique_ptr<BaseType> create_type() override;

    /**
     * this returns the return type of the function, it must be called in access chain
     * to account for generic types that depend on struct
     */
    std::unique_ptr<BaseType> create_type(std::vector<std::unique_ptr<ChainValue>>& chain, unsigned int index) override;

    /**
     * this returns the return type of the function
     */
    hybrid_ptr<BaseType> get_base_type() override;

    [[nodiscard]]
    BaseTypeKind type_kind() const override;

    [[nodiscard]]
    ValueType value_type() const override;

    /**
     * will set the current generic iteration on function declaration
     * returning the previous generic iteration, so you can restore it
     * previous iteration is equal to -2, if couldn't set because it's
     * not a generic function
     */
    int16_t set_curr_itr_on_decl(FunctionDeclaration* declaration);

    /**
     * this get's the declaration
     */
    int16_t set_curr_itr_on_decl() {
        return set_curr_itr_on_decl(safe_linked_func());
    }

    /**
     * if all generic arguments aren't given, for which default types also don't exist
     * this will be called to get inferred arguments, if parameter has default type, nullptr will be used,
     * for which arguments couldn't be inferred, nullptr would be used
     */
    void infer_generic_args(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred);

    /**
     * will infer return type (if it's generic type) for example a generic function with generic return type
     * func <T> sum(a : int, b : int) : T is called in another function func print(sum : int) like this
     * print(sum(10, 10)) <-- we know print expects a integer, we can assume sum should return integer
     */
    void infer_return_type(ASTDiagnoser& diagnoser, std::vector<BaseType*>& inferred, BaseType* expected_type);

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Type *llvm_chain_type(
            Codegen &gen,
            std::vector<std::unique_ptr<ChainValue>> &values,
            unsigned int index
    ) override;

    llvm::FunctionType *llvm_func_type(Codegen &gen) override;

    llvm::FunctionType *llvm_linked_func_type(
            Codegen& gen,
            std::vector<std::unique_ptr<ChainValue>> &chain_values,
            unsigned int index
    );

    std::pair<llvm::Value*, llvm::FunctionType*>* llvm_generic_func_data(
            std::vector<std::unique_ptr<ChainValue>> &chain_values,
            unsigned int index
    );

    llvm::Value *llvm_linked_func_callee(
            Codegen& gen,
            std::vector<std::unique_ptr<ChainValue>> &chain_values,
            unsigned int index,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles
    );

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) override;

    llvm::InvokeInst *llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind);

    llvm::Value *llvm_pointer(Codegen &gen) override;

    llvm::Value* llvm_chain_value(
            Codegen &gen,
            std::vector<llvm::Value*>& args,
            std::vector<std::unique_ptr<ChainValue>>& chain,
            unsigned int until,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            llvm::Value* returnedStruct = nullptr
    );

    /**
     * the first bool means is it dynamic, if true no further attempts at calling the function should be made
     * the second value is the result of the function call
     */
    std::pair<bool, llvm::Value*> llvm_dynamic_dispatch(
            Codegen& gen,
            std::vector<std::unique_ptr<ChainValue>> &chain_values,
            unsigned int index,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles
    );

    llvm::Value *access_chain_value(
            Codegen &gen,
            std::vector<std::unique_ptr<ChainValue>> &values,
            unsigned until,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            BaseType* expected_type
    ) override;

    llvm::Value* chain_value_with_callee(
            Codegen& gen,
            std::vector<std::unique_ptr<ChainValue>>& chain,
            unsigned int index,
            llvm::Value* grandpa_value,
            llvm::Value* callee_value,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles
    );

    llvm::AllocaInst *access_chain_allocate(
            Codegen &gen,
            std::vector<std::unique_ptr<ChainValue>> &values,
            unsigned int until,
            BaseType* expected_type
    ) override;

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