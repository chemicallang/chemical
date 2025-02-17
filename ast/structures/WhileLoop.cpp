// Copyright (c) Qinetik 2024.

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
    gen.CreateBr(loopCond);

    // loop condition
    gen.SetInsertPoint(loopCond);
    condition->llvm_conditional_branch(gen, loopThen, exitBlock);

    // loop then
    gen.SetInsertPoint(loopThen);
    gen.loop_body_gen(body, loopCond, exitBlock);
    gen.CreateBr(loopCond);

    // loop exit
    gen.SetInsertPoint(exitBlock);

}

#endif

/**
 * initializes the loop with only a condition and empty body
 * @param condition
 */
//WhileLoop::WhileLoop(std::unique_ptr<Value> condition, ASTNode* parent_node, CSTToken* token) : condition(std::move(condition)), parent_node(parent_node), token(token) {
//
//}

/**
 * @brief Construct a new WhileLoop object.
 *
 * @param condition The loop condition.
 * @param body The body of the while loop.
 */
WhileLoop::WhileLoop(Value* condition, Scope body, ASTNode* parent_node, SourceLocation location)
        : condition(condition), LoopASTNode(std::move(body)), parent_node(parent_node), location(location) {}

void WhileLoop::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    condition->link(linker, condition);
    body.link_sequentially(linker);
    linker.scope_end();
}

void WhileLoop::accept(Visitor *visitor) {
    visitor->visit(this);
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
