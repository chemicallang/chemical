// Copyright (c) Qinetik 2024.

#include "DoubleValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type *DoubleValue::llvm_type(Codegen &gen) {
    return gen.builder->getDoubleTy();
}

llvm::Value *DoubleValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), value);
}

#endif

bool DoubleValue::can_promote(Value *val) {
    return val->primitive() && val->value_type() == ValueType::Int;
}

Value* DoubleValue::promote(Value *val) {
    if(val->primitive() && val->value_type() == ValueType::Int) {
        return new DoubleValue((double) val->as_int());
    } else {
        return nullptr;
    }
}