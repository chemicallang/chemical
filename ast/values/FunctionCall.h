// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/ChainValue.h"
#include "ast/base/TypeLoc.h"
#include "ast/base/ASTNode.h"

class ASTDiagnoser;

class GenericInstantiatorAPI;

class FunctionCall : public ChainValue {
public:

    ChainValue* parent_val;
    std::vector<TypeLoc> generic_list;
    std::vector<Value*> values;

    /**
     * constructor
     */
    constexpr FunctionCall(
            ChainValue* parent,
            SourceLocation location
    ) : ChainValue(ValueKind::FunctionCall, location), parent_val(parent) {

    }

    FunctionCall(FunctionCall &&other) = delete;

    uint64_t byte_size(bool is64Bit) final;

    void link_values(SymbolResolver &linker, std::vector<bool>& properly_linked);

    /**
     * if an arg has implicit constructor, we call that implicit constructor instead of
     * just passing that value
     */
    void link_args_implicit_constructor(SymbolResolver &linker, std::vector<bool>& properly_linked);

    bool link_gen_args(SymbolResolver &linker);

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final {
        if(!parent_val->link(linker, (Value*&) parent_val, nullptr)) {
            return false;
        }
        return link_without_parent(linker, expected_type, true);
    }

    /**
     * get function type from parent type
     */
    FunctionType* func_type_from_parent_type(ASTAllocator& allocator, BaseType* parent_type);

    /**
     * get the function type
     */
    FunctionType* function_type(ASTAllocator& allocator);

    /**
     * get known func type
     */
    FunctionType* known_func_type();

    /**
     * known type of function call's return
     */
    BaseType* known_type() final;

    /**
     * argument type
     */
    BaseType* get_arg_type(unsigned int index);

    ASTNode *linked_node() final;

    void relink_multi_func(ASTAllocator& allocator, ASTDiagnoser* diagnoser);

    void link_constructor(ASTAllocator& allocator, GenericInstantiatorAPI& genApi);

    bool instantiate_gen_call(GenericInstantiatorAPI& genApi, BaseType* expected_type);

    bool link_without_parent(
            SymbolResolver &resolver,
            BaseType *expected_type,
            bool link_implicit_constructor
    );

    bool primitive() final;

    bool compile_time_computable() final;

    Value *find_in(InterpretScope &scope, Value *parent) final;

    Value* evaluated_value(InterpretScope &scope) final;

    FunctionCall *copy(ASTAllocator& allocator) final;

    /**
     * this returns the return type of the function
     */
    BaseType* create_type(ASTAllocator& allocator) final;

    /**
     * if all generic arguments aren't given, for which default types also don't exist
     * this will be called to get inferred arguments, if parameter has default type, nullptr will be used,
     * for which arguments couldn't be inferred, nullptr would be used
     */
    void infer_generic_args(ASTAllocator& allocator, ASTDiagnoser& diagnoser, std::vector<TypeLoc>& inferred);

    /**
     * will infer return type (if it's generic type) for example a generic function with generic return type
     * func <T> sum(a : int, b : int) : T is called in another function func print(sum : int) like this
     * print(sum(10, 10)) <-- we know print expects a integer, we can assume sum should return integer
     */
    void infer_return_type(ASTDiagnoser& diagnoser, std::vector<TypeLoc>& inferred, BaseType* expected_type);

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(
            Codegen &gen,
            std::vector<ChainValue*> &values,
            unsigned int index
    ) final;

    llvm::FunctionType *llvm_linked_func_type(Codegen& gen);

    llvm::Value *llvm_linked_func_callee(Codegen& gen);

    llvm::InvokeInst *llvm_invoke(Codegen &gen, llvm::BasicBlock* normal, llvm::BasicBlock* unwind);

    llvm::Value *llvm_pointer(Codegen &gen) final;

    llvm::Value *llvm_value(Codegen &gen, BaseType *type = nullptr) override;

    llvm::Value* llvm_chain_value(
            Codegen &gen,
            std::vector<llvm::Value*>& args,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles,
            llvm::Value* returnedStruct = nullptr,
            llvm::Value* callee_value = nullptr,
            llvm::Value* grandparent = nullptr
    );

    llvm::Value* llvm_chain_value(
            Codegen &gen,
            llvm::Value* returnedStruct = nullptr,
            llvm::Value* callee_value = nullptr,
            llvm::Value* grandparent = nullptr
    );

    bool store_in_parent(
        Codegen &gen,
        llvm::Value* allocated,
        llvm::Type* allocated_type,
        std::vector<llvm::Value*>& idxList,
        unsigned int index
    );

    unsigned int store_in_struct(
        Codegen &gen,
        Value *parent,
        llvm::Value *allocated,
        llvm::Type *allocated_type,
        std::vector<llvm::Value *> idxList,
        unsigned int index,
        BaseType *expected_type
    ) override {
        if(store_in_parent(gen, allocated, allocated_type, idxList, index)) {
            return index + 1;
        }
        return Value::store_in_array(gen, parent, allocated, allocated_type, idxList, index, expected_type);
    }

    unsigned int store_in_array(
            Codegen &gen,
            Value *parent,
            llvm::Value *allocated,
            llvm::Type *allocated_type,
            std::vector<llvm::Value *> idxList,
            unsigned int index,
            BaseType *expected_type
    ) override {
        if(store_in_parent(gen, allocated, allocated_type, idxList, index)) {
            return index + 1;
        }
        return Value::store_in_array(gen, parent, allocated, allocated_type, idxList, index, expected_type);
    }

    /**
     * the first bool means is it dynamic, if true no further attempts at calling the function should be made
     * the second value is the result of the function call
     */
    std::pair<bool, llvm::Value*> llvm_dynamic_dispatch(
            Codegen& gen,
            std::vector<std::pair<Value*, llvm::Value*>>& destructibles
    );

    llvm::AllocaInst *access_chain_allocate(
            Codegen &gen,
            std::vector<ChainValue*> &values,
            unsigned int until,
            BaseType* expected_type
    ) final;

    void access_chain_assign_value(
            Codegen &gen,
            AccessChain* chain,
            unsigned int until,
            std::vector<std::pair<Value*, llvm::Value*>> &destructibles,
            llvm::Value* lhsPtr,
            Value *lhs,
            BaseType *expected_type
    ) final;

    bool add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) final;

#endif

    /**
     * get linked node as a function
     * you should call this when you are sure, that this call is to a function
     * which is a function declaration
     */
    [[nodiscard]] inline FunctionDeclaration* linked_func() const {
        return parent_val->linked_node()->as_function();
    }

    /**
     * if this call refers to a function declaration, returns it, otherwise not
     * so its safe
     */
    [[nodiscard]] inline FunctionDeclaration* safe_linked_func() const {
        const auto linked = parent_val->linked_node();
        return linked ? linked->as_function() : nullptr;
    }

};