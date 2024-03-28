// Copyright (c) Qinetik 2024.

#include "Value.h"
#include "compiler/llvmimpl.h"

void Value::llvm_allocate(Codegen& gen, const std::string& identifier) {
    auto x = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
    gen.allocated[identifier] = x;
    gen.builder->CreateStore(llvm_value(gen), x);
}