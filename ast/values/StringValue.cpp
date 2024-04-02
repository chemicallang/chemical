// Copyright (c) Qinetik 2024.

#include "StringValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Type *StringValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8PtrTy();
}

llvm::Value * StringValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateGlobalStringPtr(value);
}

llvm::GlobalVariable * StringValue::llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) {
    return gen.builder->CreateGlobalString(value, name);
}

#endif