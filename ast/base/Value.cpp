// Copyright (c) Qinetik 2024.

#include "Value.h"
#include "ast/values/StructValue.h"
#include "ast/values/ArrayValue.h"
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
        Codegen& gen,
        StructValue* parent,
        llvm::AllocaInst* ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    auto elementPtr = gen.builder->CreateGEP(parent->llvm_type(gen), ptr, idxList, "", gen.inbounds);
    gen.builder->CreateStore(llvm_value(gen), elementPtr);
    return index + 1;
}

unsigned int Value::store_in_array(
        Codegen& gen,
        ArrayValue* parent,
        llvm::AllocaInst* ptr,
        std::vector<llvm::Value *> idxList,
        unsigned int index
) {
    idxList.emplace_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*gen.ctx), index));
    auto elemPtr = gen.builder->CreateGEP(parent->llvm_type(gen), ptr, idxList, "", gen.inbounds);
    gen.builder->CreateStore(llvm_value(gen), elemPtr);
    return index + 1;
}

#endif