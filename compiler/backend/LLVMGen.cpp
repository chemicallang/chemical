// Copyright (c) Chemical Language Foundation 2025.

#include "LLVMGen.h"
#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"
#include "compiler/backend/DebugInfoBuilder.h"

inline llvm::ArrayRef<llvm::Value*> ref(const std::span<llvm::Value*>& idxList) {
    return {idxList.data(), idxList.size()};
}

llvm::Value* LLVMGenCreateAlloca(LLVMGen* gen, llvm::Type* type, SourceLocation location) {
    const auto allocaInst = gen->builder->CreateAlloca(type);
    gen->di.instr(allocaInst, location);
    return allocaInst;
}

llvm::StoreInst* LLVMGenCreateStore(LLVMGen* gen, llvm::Value* val, llvm::Value* ptr, SourceLocation location) {
    const auto storeInst = gen->builder->CreateStore(val, ptr);
    gen->di.instr(storeInst, location);
    return storeInst;
}

llvm::Value* LLVMGenCreateGEP(LLVMGen* gen, llvm::Type* type, llvm::Value* ptr, const std::span<llvm::Value*>& idxList) {
    return gen->builder->CreateGEP(type, ptr, ref(idxList));
}

llvm::Value* LLVMGenCreateStructGEP(LLVMGen* gen, llvm::Type* type, llvm::Value* ptr, unsigned int idx) {
    return gen->builder->CreateStructGEP(type, ptr, idx);
}