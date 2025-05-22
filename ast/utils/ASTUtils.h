// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <memory>
#include "ast/base/ChainValue.h"
#include "ast/base/TypeLoc.h"

bool has_function_call_before(ChainValue* value);

ChainValue* get_parent_from(ChainValue* value);

ChainValue* get_grandpa_from(ChainValue* value);

ChainValue* build_parent_chain(std::vector<ChainValue*>& values, ASTAllocator& allocator);

ChainValue* build_parent_chain(ChainValue* value, ASTAllocator& allocator);

void evaluate_values(std::vector<Value*>& values, InterpretScope& scope);

/**
 * call the given function declaration with given argument
 * doesn't link the value according to implicit constructor type
 */
FunctionCall* call_with_arg(FunctionDeclaration* decl, Value* arg, BaseType* expected_type, ASTAllocator& allocator, ASTDiagnoser& diagnoser);

void infer_generic_args(
    std::vector<TypeLoc>& out_generic_args,
    std::vector<GenericTypeParameter*>& generic_params,
    FunctionCall* call,
    ASTDiagnoser& diagnoser,
    BaseType* expected_type
);

/**
 * when the given value for the given expected type, has a constructor
 */
void link_with_implicit_constructor(FunctionDeclaration* decl, SymbolResolver& resolver, Value* value);