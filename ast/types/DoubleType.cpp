// Copyright (c) Qinetik 2024.

#include "DoubleType.h"
#include "ast/values/DoubleValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *DoubleType::llvm_type(Codegen &gen) const {
    return gen.builder->getDoubleTy();
}

#endif

std::unique_ptr<Value> DoubleType::promote(Value* value) {
    if(value->value_type() == ValueType::Int) {
        return std::make_unique<DoubleValue>((double) value->as_int());
    } else {
        return std::unique_ptr<Value>(value);
    }
}