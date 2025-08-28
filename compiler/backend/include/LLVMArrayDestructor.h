// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/llvmfwd.h"

class Codegen;

class LLVMArrayDestructor {
public:

    Codegen& gen;
    llvm::Value* const structPtr;
    llvm::Value* const firstEle;
    llvm::BasicBlock* const body_block;
    llvm::BasicBlock* const end_block;

    LLVMArrayDestructor(
            Codegen& gen,
            llvm::Value* structPtr,
            llvm::Value* firstEle,
            llvm::BasicBlock* body_block,
            llvm::BasicBlock* end_block
    ) : gen(gen), structPtr(structPtr), firstEle(firstEle), body_block(body_block), end_block(end_block) {

    }

    ~LLVMArrayDestructor();

};