// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "ast/base/ChainValue.h"

bool chain_contains_func_call(std::vector<std::unique_ptr<ChainValue>>& values, int start, int end);

void evaluate_values(std::vector<std::unique_ptr<Value>>& values, InterpretScope& scope);

/**
 * call the given function declaration with given argument
 */
std::unique_ptr<Value> call_with_arg(FunctionDeclaration* decl, std::unique_ptr<Value> arg, SymbolResolver& resolver);