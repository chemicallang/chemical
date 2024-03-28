// Copyright (c) Qinetik 2024.

#include "StructValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::Value *StructValue::llvm_pointer(Codegen &gen) {
    return allocaInst;
}

void StructValue::llvm_allocate(Codegen &gen, const std::string &identifier) {
    allocaInst = gen.builder->CreateAlloca(llvm_type(gen), nullptr, structName);
}

llvm::Value *StructValue::llvm_value(Codegen &gen) {
    throw std::runtime_error("cannot allocate an array without an identifier");
}

llvm::Type *StructValue::llvm_elem_type(Codegen &gen) {
    throw std::runtime_error("cannot allocate an array without an identifier");
}

llvm::Type *StructValue::llvm_type(Codegen &gen) {
    return llvm::StructType::get(*gen.ctx, definition->elements_type(gen));
}

#endif