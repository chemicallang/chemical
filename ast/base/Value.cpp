// Copyright (c) Qinetik 2024.

#include "Value.h"
#include "ast/values/StructValue.h"
#include "ast/structures/StructDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

llvm::AllocaInst *Value::llvm_allocate(Codegen &gen, const std::string &identifier) {
    auto x = gen.builder->CreateAlloca(llvm_type(gen), nullptr, identifier);
    gen.builder->CreateStore(llvm_value(gen), x);
    return x;
}

llvm::GlobalVariable* Value::llvm_global_variable(Codegen& gen, bool is_const, const std::string& name) {
    return new llvm::GlobalVariable(*gen.module, llvm_type(gen), is_const, llvm::GlobalValue::LinkageTypes::PrivateLinkage, (llvm::Constant*) llvm_value(gen), name);
}

unsigned int Value::store_in_struct(
        Codegen &gen,
        StructValue *parent,
        llvm::AllocaInst *ptr,
        const std::string &identifier,
        unsigned int index
) {
    auto child = parent->definition->child(identifier);
    std::vector<llvm::Value *> childIdx;
    childIdx.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    auto elementPtr = gen.builder->CreateGEP(child->llvm_type(gen), ptr, childIdx);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

#endif