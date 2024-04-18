// Copyright (c) Qinetik 2024.

#include "FloatType.h"
#include "ast/values/FloatValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *FloatType::llvm_type(Codegen &gen) const {
    return gen.builder->getFloatTy();
}

#endif

std::unique_ptr<Value> FloatType::promote(Value* value) {
    if(value->value_type() == ValueType::Int) {
        return std::make_unique<FloatValue>((float) value->as_int());
    } else {
        return std::unique_ptr<Value>(value);
    }
}