// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "ast/base/ChainValue.h"

bool chain_contains_func_call(std::vector<ChainValue*>& values, int start, int end);

void evaluate_values(std::vector<Value*>& values, InterpretScope& scope);

/**
 * call the given function declaration with given argument
 * doesn't link the value according to implicit constructor type
 */
Value* call_with_arg(FunctionDeclaration* decl, Value* arg, ASTAllocator& allocator);

/**
 * call the given function declaration with given argument
 */
Value* call_with_arg(FunctionDeclaration* decl, Value* arg, SymbolResolver& resolver);

/**
 * when the given value for the given expected type, has a constructor
 */
void link_with_implicit_constructor(FunctionDeclaration* decl, SymbolResolver& resolver, Value* value);