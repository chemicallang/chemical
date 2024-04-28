// Copyright (c) Qinetik 2024.

#include "WhileLoop.h"

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

/**
 * initializes the loop with only a condition and empty body
 * @param condition
 */
WhileLoop::WhileLoop(std::unique_ptr<Value> condition) : condition(std::move(condition)) {

}

/**
 * @brief Construct a new WhileLoop object.
 *
 * @param condition The loop condition.
 * @param body The body of the while loop.
 */
WhileLoop::WhileLoop(std::unique_ptr<Value> condition, LoopScope body)
        : condition(std::move(condition)), LoopASTNode(std::move(body)) {}

void WhileLoop::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    condition->link(linker);
    body.declare_and_link(linker);
    linker.scope_end();
}

void WhileLoop::accept(Visitor &visitor) {
    visitor.visit(this);
}

void WhileLoop::interpret(InterpretScope &scope) {
    InterpretScope child(&scope, scope.global, &body, this);
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

std::string WhileLoop::representation() const {
    std::string ret;
    ret.append("while(");
    ret.append(condition->representation());
    ret.append(") {\n");
    ret.append(body.representation());
    ret.append("\n}");
    return ret;
}
