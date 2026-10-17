// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 29/02/2024.
//

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"
#include "ast/base/ASTNode.h"

class ASTDiagnoser;

class GenericInstantiatorAPI;


#include "ast/structures/BaseGenericDecl.h"

class FunctionCall : public Value {
public:

    Value* parent_val;
    std::vector<TypeLoc> generic_list;
    std::vector<Value*> values;

    /**
     * constructor
     */
    constexpr FunctionCall(
            Value* parent,
            SourceLocation location
    ) : Value(ValueKind::FunctionCall, location), parent_val(parent) {

    }

    /**
     * constructor
     */
    constexpr FunctionCall(
            Value* parent,
            BaseType* type,
            SourceLocation location
    ) : Value(ValueKind::FunctionCall, type, location), parent_val(parent) {

    }

    FunctionCall(FunctionCall &&other) = delete;

    uint64_t byte_size(const TargetData& target) final;

    void report_concrete_types(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    /**
     * get function type from parent type
     */
    FunctionType* func_type_from_parent_type(BaseType* parent_type);

    /**
     * the replacement for function_type with allocator
     */
    FunctionType* function_type();

    /**
     * gets the function type during linking, this also checks for calls to structs and variants
     */
    FunctionType* get_function_type_during_linking();

    /**
     * get known func type
     */
    FunctionType* known_func_type();

    void determine_type(ASTAllocator& allocator, ASTDiagnoser& diagnoser);

    /**
     * argument type
     */
    ASTNode *linked_node() final;


    void link_constructor(ASTAllocator& allocator, GenericInstantiatorAPI& genApi, bool specialize_generic, InstantiationRequirement requirement);

    bool instantiate_gen_call(GenericInstantiatorAPI& genApi, BaseType* expected_type, InstantiationRequirement requirement);

    bool primitive() final;

    bool compile_time_computable() final;

    Value* evaluated_value(InterpretScope &scope) final;

    FunctionCall *copy(ASTAllocator& allocator) final;

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