// Copyright (c) Qinetik 2024.

#include "BaseType.h"
#include "Value.h"

std::unique_ptr<Value> BaseType::promote(Value* value) {
    return std::unique_ptr<Value>(value);
}