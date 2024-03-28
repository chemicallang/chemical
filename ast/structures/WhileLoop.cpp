// Copyright (c) Qinetik 2024.

#include "WhileLoop.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"

void WhileLoop::code_gen(Codegen &gen) {

    auto loopCond = llvm::BasicBlock::Create(*gen.ctx, "loopcond", gen.current_function);
    auto loopThen = llvm::BasicBlock::Create(*gen.ctx, "loopthen", gen.current_function);
    auto exitBlock = llvm::BasicBlock::Create(*gen.ctx, "loopexit", gen.current_function);

    // sending to loop condition
    gen.CreateBr(loopCond);

    // loop condition
    gen.SetInsertPoint(loopCond);
    auto comparison = condition->llvm_value(gen);
    gen.CreateCondBr(comparison, loopThen, exitBlock);

    // loop then
    gen.SetInsertPoint(loopThen);
    gen.loop_body_wrap(loopCond, exitBlock);
    body.code_gen(gen);
    gen.loop_body_wrap(loopCond, exitBlock);
    gen.CreateBr(loopCond);

    // loop exit
    gen.SetInsertPoint(exitBlock);

}

#endif