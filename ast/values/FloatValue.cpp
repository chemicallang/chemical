// Copyright (c) Qinetik 2024.

#include "FloatValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type * FloatValue::llvm_type(Codegen &gen) {
    return gen.builder->getFloatTy();
}

llvm::Value * FloatValue::llvm_value(Codegen &gen) {
    return llvm::ConstantFP::get(llvm_type(gen), llvm::APFloat(value));
}

#endif

bool FloatValue::can_promote(Value *val) {
    return val->primitive() && val->value_type() == ValueType::Int;
}

Value *FloatValue::promote(Value *val) {
    if(val->primitive() && val->value_type() == ValueType::Int) {
        return new FloatValue((float) val->as_int());
    } else {
        return nullptr;
    }
}