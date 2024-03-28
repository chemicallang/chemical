// Copyright (c) Qinetik 2024.

#include "ForLoop.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void ForLoop::code_gen(Codegen &gen) {

    // initialize the variables
    initializer->code_gen(gen);

    // creating blocks
    auto condBlock = llvm::BasicBlock::Create(*gen.ctx, "forcond", gen.current_function);
    auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "forthen", gen.current_function);
    auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forend", gen.current_function);

    // going to condition
    gen.CreateBr(condBlock);

    // condition block
    gen.SetInsertPoint(condBlock);
    auto comparison = conditionExpr->llvm_value(gen);
    gen.CreateCondBr(comparison, thenBlock, endBlock);

    // then block
    gen.SetInsertPoint(thenBlock);
    gen.loop_body_wrap(condBlock, endBlock);
    body.code_gen(gen);
    gen.loop_body_wrap(condBlock, endBlock);
    incrementerExpr->code_gen(gen);
    gen.CreateBr(condBlock);

    // end block
    gen.SetInsertPoint(endBlock);

}

#endif