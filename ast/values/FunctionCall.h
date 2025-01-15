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

    ChainValue* parent_val;
    std::vector<BaseType*> generic_list;
    std::vector<Value*> values;
    int16_t generic_iteration = -1;
    SourceLocation location;

    FunctionCall(
            ChainValue* parent,
            std::vector<Value*> values,
            SourceLocation location
    ) : parent_val(parent), values(std::move(values)), location(location) {

    }

    FunctionCall(FunctionCall &&other) = delete;

    SourceLocation encoded_location() final {
        return location;
    }

    ValueKind val_kind() final {
        return ValueKind::FunctionCall;
    }

    uint64_t byte_size(bool is64Bit) final;

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void link_values(SymbolResolver &linker, std::vector<bool>& properly_linked);

    /**
     * this should be called only after generic types are known
     */
    void relink_values(SymbolResolver &linker);

    /**
     * if an arg has implicit constructor, we call that implicit constructor instead of
     * just passing that value
     */
    void link_args_implicit_constructor(SymbolResolver &linker, std::vector<bool>& properly_linked);

    void link_gen_args(SymbolResolver &linker);

    bool link(SymbolResolver &linker, Value*& value_ptr, BaseType *expected_type = nullptr) final;

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

    void fix_generic_iteration(ASTDiagnoser& diagnoser, BaseType* expected_type);

    /**
     * called by the current function (in the body of which this function call exists)
     * this means the current function is generic, and this call also calls a generic function
     * and a generic iteration has been registered for current function, which means we must register
     * a generic iteration of the called function
     */
    int16_t register_indirect_generic_iteration(ASTAllocator& astAllocator, ASTDiagnoser& diagnoser, BaseType* expected_type);

    void relink_multi_func(ASTAllocator& allocator, ASTDiagnoser* diagnoser);

    int16_t link_constructor(ASTAllocator& allocator, ASTAllocator& astAllocator, ASTDiagnoser& diagnoser);

    bool find_link_in_parent(
            SymbolResolver &resolver,
            BaseType *expected_type,
            bool link_implicit_constructor
    );

    bool link(
        SymbolResolver &linker,
        std::vector<ChainValue*> &values,
        unsigned int index,
        BaseType *expected_type
    ) final {
        return link(linker, (Value*&) values[index], expected_type);
    }

    void relink_parent(ChainValue *parent) final;

    bool primitive() final;

    bool compile_time_computable() final;

    Value *find_in(InterpretScope &scope, Value *parent) final;

    Value* evaluated_value(InterpretScope &scope) final;

    Value* evaluated_chain_value(InterpretScope &scope, Value *parent) final;

    FunctionCall *copy(ASTAllocator& allocator) final;

    void interpret(InterpretScope &scope) final;

    /**
     * this returns the return type of the function
     */
    BaseType* create_type(ASTAllocator& allocator) final;

//    /**
//     * this returns the return type of the function, it must be called in access chain
//     * to account for generic types that depend on struct
//     */
//    BaseType* create_type(std::vector<ChainValue*>& chain, unsigned int index) final;

    [[nodiscard]]
    BaseTypeKind type_kind() const final;

    [[nodiscard]]
    ValueType value_type() const final;

    /**
     * will set given generic iteration of function declaration
     * if function is a constructor, will set the iteration on the struct instead
     * previous iteration of declaration is returned
     */
    int16_t set_gen_itr_on_decl(int16_t itr, bool set_generic_calls = true);

    /**
     * will set the current generic iteration on function declaration
     * returning the previous generic iteration, so you can restore it
     * previous iteration is equal to -2, if couldn't set because it's
     * not a generic function
     */
    inline int16_t set_curr_itr_on_decl(bool set_generic_calls = true) {
        return set_gen_itr_on_decl(generic_iteration, set_generic_calls);
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

    llvm::Type *llvm_type(Codegen &gen) final;

    llvm::Type *llvm_chain_type(
            Codegen &gen,
            std::vector<ChainValue*> &values,
            unsigned int index
    ) final;

    llvm::FunctionType *llvm_linked_func_type(Codegen& gen);

    std::pair<llvm::Value*, llvm::FunctionType*>* llvm_generic_func_data(ASTAllocator& allocator);

    llvm::Value *llvm_linked_func_callee(Codegen& gen);

    void llvm_destruct(Codegen &gen, llvm::Value *allocaInst) final;

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

    llvm::Value* access_chain_assign_value(
            Codegen &gen,
            std::vector<ChainValue*> &values,
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