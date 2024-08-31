// Copyright (c) Qinetik 2024.

#include "DoWhileLoop.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void DoWhileLoop::code_gen(Codegen &gen) {

    auto loopThen = llvm::BasicBlock::Create(*gen.ctx, "loopthen", gen.current_function);
    auto loopCond = llvm::BasicBlock::Create(*gen.ctx, "loopcond", gen.current_function);
    auto exitBlock = llvm::BasicBlock::Create(*gen.ctx, "loopexit", gen.current_function);

    // sending to loop then
    gen.CreateBr(loopThen);

    // loop then
    gen.SetInsertPoint(loopThen);
    gen.loop_body_gen(body, loopCond, exitBlock);
    gen.CreateBr(loopCond);

    // loop condition
    gen.SetInsertPoint(loopCond);
    condition->llvm_conditional_branch(gen, loopThen, exitBlock);

    // loop exit
    gen.SetInsertPoint(exitBlock);

}

#endif

/**
 * @brief Construct a new WhileLoop object.
 *
 * @param condition The loop condition.
 * @param body The body of the while loop.
 */
DoWhileLoop::DoWhileLoop(std::unique_ptr<Value> condition, LoopScope body)
        : condition(std::move(condition)), LoopASTNode(std::move(body)) {}

void DoWhileLoop::accept(Visitor *visitor) {
    visitor->visit(this);
}

void DoWhileLoop::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    linker.scope_start();
    body.link_sequentially(linker);
    condition->link(linker, condition);
    linker.scope_end();
}

void DoWhileLoop::interpret(InterpretScope &scope) {
    InterpretScope child(&scope, scope.global);
    do {
        body.interpret(child);
        if (stoppedInterpretation) {
            stoppedInterpretation = false;
            break;
        }
    } while (condition->evaluated_bool(child));
}

void DoWhileLoop::stopInterpretation() {
    stoppedInterpretation = true;
}
