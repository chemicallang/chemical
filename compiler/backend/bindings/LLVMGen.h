// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/llvmfwd.h"
#include "compiler/clangfwd.h"
#include <span>
#include "core/source/SourceLocation.h"

class LLVMGen;

class DebugInfoBuilder;

extern "C" {

    llvm::Value* LLVMGenCreateAlloca(LLVMGen* gen, llvm::Type* type, SourceLocation location);

    llvm::StoreInst* LLVMGenCreateStore(LLVMGen* gen, llvm::Value* val, llvm::Value* ptr, SourceLocation location);

    llvm::Value* LLVMGenCreateGEP(LLVMGen* gen, llvm::Type* type, llvm::Value* ptr, const std::span<llvm::Value*>& idxList);

    llvm::Value* LLVMGenCreateStructGEP(LLVMGen* gen, llvm::Type* type, llvm::Value* ptr, unsigned int idx);

}

class LLVMGen {
public:

    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter> *builder;

    DebugInfoBuilder& di;

    /**
     * constructor
     */
    LLVMGen(DebugInfoBuilder& di) : di(di) {

    }

    void updateBuilder(llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>* newBuilder) {
        builder = newBuilder;
    }

    /**
     * create alloca instruction
     */
    inline llvm::Value* CreateAlloca(llvm::Type* type, SourceLocation location) {
        return LLVMGenCreateAlloca(this, type, location);
    }

    /**
     * create store instruction
     */
    inline llvm::StoreInst* CreateStore(llvm::Value* val, llvm::Value* ptr, SourceLocation location) {
        return LLVMGenCreateStore(this, val, ptr, location);
    }

    /**
     * Create GEP
     */
    llvm::Value* CreateGEP(llvm::Type* type, llvm::Value* ptr, const std::span<llvm::Value*>& idxList) {
        return LLVMGenCreateGEP(this, type, ptr, idxList);
    }

    /**
     * create struct gep
     */
    llvm::Value* CreateStructGEP(llvm::Type* type, llvm::Value* ptr, unsigned int idx) {
        return LLVMGenCreateStructGEP(this, type, ptr, idx);
    }

    /**
     * helper method
     */
    template <typename NodeT>
    inline llvm::Value* CreateAlloca(llvm::Type* type, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return CreateAlloca(type, node->encoded_location());
    }

    /**
     * helper method
     */
    template <typename NodeT>
    inline llvm::StoreInst* CreateStore(llvm::Value* val, llvm::Value* ptr, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        return CreateStore(val, ptr, node->encoded_location());
    }

};