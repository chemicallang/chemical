// Copyright (c) Chemical Language Foundation 2025.

#include "WhileLoop.h"
#include "ast/base/InterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void WhileLoop::code_gen(Codegen &gen) {

    auto loopCond = llvm::BasicBlock::Create(*gen.ctx, "loopcond", gen.current_function);
    auto loopThen = llvm::BasicBlock::Create(*gen.ctx, "loopthen", gen.current_function);
    auto exitBlock = llvm::BasicBlock::Create(*gen.ctx, "loopexit", gen.current_function);

    // sending to loop condition
    gen.CreateBr(loopCond, encoded_location());

    // loop condition
    gen.SetInsertPoint(loopCond);
    condition->llvm_conditional_branch(gen, loopThen, exitBlock);

    // loop then
    gen.SetInsertPoint(loopThen);
    gen.loop_body_gen(body, loopCond, exitBlock);

    // TODO use the ending location
    gen.CreateBr(loopCond, body.encoded_location());

    // loop exit
    gen.SetInsertPoint(exitBlock);

}

#endif

void WhileLoop::stopInterpretation() {
    stoppedInterpretation = true;
}
