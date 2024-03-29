// Copyright (c) Qinetik 2024.

#include "Value.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::AllocaInst* Value::llvm_allocate(Codegen& gen, const std::string& identifier) {
    auto x = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
    gen.builder->CreateStore(llvm_value(gen), x);
    return x;
}

#endif