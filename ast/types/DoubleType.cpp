// Copyright (c) Qinetik 2024.

#include "DoubleType.h"
#include "ast/values/DoubleValue.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type *DoubleType::llvm_type(Codegen &gen) const {
    return gen.builder->getDoubleTy();
}

#endif

bool DoubleType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *DoubleType::promote(Value *value) {
    if(value->primitive() && value->value_type() == ValueType::Int) {
        return new DoubleValue((double) value->as_int());
    } else {
        return nullptr;
    }
}