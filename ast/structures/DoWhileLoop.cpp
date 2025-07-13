// Copyright (c) Chemical Language Foundation 2025.

#include "DoWhileLoop.h"
#include "ast/base/InterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void DoWhileLoop::code_gen(Codegen &gen) {

    auto loopThen = llvm::BasicBlock::Create(*gen.ctx, "loopthen", gen.current_function);
    auto loopCond = llvm::BasicBlock::Create(*gen.ctx, "loopcond", gen.current_function);
    auto exitBlock = llvm::BasicBlock::Create(*gen.ctx, "loopexit", gen.current_function);

    // sending to loop then
    gen.CreateBr(loopThen, body.encoded_location());

    // loop then
    gen.SetInsertPoint(loopThen);
    gen.loop_body_gen(body, loopCond, exitBlock);
    gen.CreateBr(loopCond, body.encoded_location());

    // loop condition
    gen.SetInsertPoint(loopCond);
    condition->llvm_conditional_branch(gen, loopThen, exitBlock);

    // loop exit
    gen.SetInsertPoint(exitBlock);

}

#endif

void DoWhileLoop::stopInterpretation() {
    stoppedInterpretation = true;
}
