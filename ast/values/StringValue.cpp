// Copyright (c) Qinetik 2024.

#include "StringValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type *StringValue::llvm_type(Codegen &gen) {
    return gen.builder->getInt8PtrTy();
}

llvm::Value * StringValue::llvm_value(Codegen &gen) {
    return gen.builder->CreateGlobalStringPtr(value);
}

llvm::GlobalVariable * StringValue::llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) {
    if(!is_const) {
        gen.error("Global string variables aren't supported at the moment");
    }
    // TODO global constant string must have type pointer to array
    // because it returns an array pointer, and we must take [0] from it to reach first pointer
    return gen.builder->CreateGlobalString(value, name, 0, gen.module.get());
}

#endif