// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include <memory>
#include "ast/base/Value.h"

bool chain_contains_func_call(std::vector<std::unique_ptr<Value>>& values, unsigned start, unsigned end);