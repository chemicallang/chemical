// Copyright (c) Qinetik 2024.

#include "Codegen.h"

void Codegen::SetInsertPoint(llvm::BasicBlock* block) {
    has_current_block_ended = false;
    builder->SetInsertPoint(block);
}

void Codegen::CreateBr(llvm::BasicBlock* block) {
    if(!has_current_block_ended) {
        builder->CreateBr(block);
        has_current_block_ended = true;
    }
}