// Copyright (c) Chemical Language Foundation 2025.

#include "ForLoop.h"
#include "compiler/SymbolResolver.h"

#ifdef COMPILER_BUILD

#include "compiler/Codegen.h"
#include "compiler/llvmimpl.h"

void ForLoop::code_gen(Codegen &gen) {

    // initialize the variables
    initializer->code_gen(gen);

    // creating blocks
    auto condBlock = llvm::BasicBlock::Create(*gen.ctx, "forcond", gen.current_function);
    auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "forthen", gen.current_function);
    auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "forend", gen.current_function);

    // going to condition
    gen.CreateBr(condBlock, body.encoded_location());

    // condition block
    gen.SetInsertPoint(condBlock);
    auto comparison = conditionExpr->llvm_value(gen);
    gen.CreateCondBr(comparison, thenBlock, endBlock, body.encoded_location());

    // then block
    gen.SetInsertPoint(thenBlock);
    gen.loop_body_gen(body, condBlock, endBlock);
    incrementerExpr->code_gen(gen);
    // TODO use the ending location here
    gen.CreateBr(condBlock, body.encoded_location());

    // end block
    gen.SetInsertPoint(endBlock);

}

#endif

void ForLoop::stopInterpretation() {
    stoppedInterpretation = true;
}