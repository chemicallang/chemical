// Copyright (c) Qinetik 2024.

#include "FloatType.h"
#include "ast/values/FloatValue.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

llvm::Type *FloatType::llvm_type(Codegen &gen) const {
    return gen.builder->getFloatTy();
}

#endif

bool FloatType::can_promote(Value *value) {
    return value->primitive() && value->value_type() == ValueType::Int;
}

Value *FloatType::promote(Value *value) {
    if(value->primitive() && value->value_type() == ValueType::Int) {
        return new FloatValue((float) value->as_int());
    } else {
        return nullptr;
    }
}