// Copyright (c) Chemical Language Foundation 2025.

#include "WhileLoop.h"
#include "compiler/SymbolResolver.h"
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

/**
 * initializes the loop with only a condition and empty body
 * @param condition
 */
//WhileLoop::WhileLoop(std::unique_ptr<Value> condition, ASTNode* parent_node, CSTToken* token) : condition(std::move(condition)), token(token) {
//
//}

void WhileLoop::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    condition->link(linker, condition);
    body.link_sequentially(linker);
    linker.scope_end();
}

void WhileLoop::interpret(InterpretScope &scope) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    while (condition->evaluated_bool(child)) {
        body.interpret(child);
        if (stoppedInterpretation) {
            stoppedInterpretation = false;
            break;
        }
    }
}

void WhileLoop::stopInterpretation() {
    stoppedInterpretation = true;
}
