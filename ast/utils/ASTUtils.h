// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"
#include "ast/base/TypeLoc.h"

bool has_function_call_before(Value* value);

VariableIdentifier* get_first_chain_id(Value* value);

inline VariableIdentifier* get_first_chain_value(Value* value) {
    return get_first_chain_id(value);
}

Value* get_parent_from(Value* value);

Value* get_grandpa_from(Value* value);

Value* build_parent_chain(std::vector<Value*>& values, ASTAllocator& allocator);

Value* build_parent_chain(Value* value, ASTAllocator& allocator);

void evaluate_values(std::vector<Value*>& values, InterpretScope& scope);

/**
 * call the given function declaration with given argument
 * doesn't link the value according to implicit constructor type
 */
FunctionCall* call_with_arg(FunctionDeclaration* decl, Value* arg, BaseType* expected_type, ASTAllocator& allocator, ASTDiagnoser& diagnoser);

void infer_generic_args(
    ASTAllocator& allocator,
    std::vector<TypeLoc>& out_generic_args,
    std::vector<GenericTypeParameter*>& generic_params,
    FunctionCall* call,
    ASTDiagnoser& diagnoser,
    BaseType* expected_type
);