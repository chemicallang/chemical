// Copyright (c) Qinetik 2024.

#include "DoWhileLoop.h"
#include "compiler/SymbolResolver.h"

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
    gen.loop_body_wrap(loopCond, exitBlock);
    body.code_gen(gen);
    gen.loop_body_wrap(loopCond, exitBlock);
    gen.CreateBr(loopCond);

    // loop condition
    gen.SetInsertPoint(loopCond);
    auto comparison = condition->llvm_value(gen);
    gen.CreateCondBr(comparison, loopThen, exitBlock);

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

void DoWhileLoop::accept(Visitor &visitor) {
    visitor.visit(this);
}

void DoWhileLoop::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    body.declare_and_link(linker);
    condition->link(linker);
    linker.scope_end();
}

void DoWhileLoop::interpret(InterpretScope &scope) {
    InterpretScope child(&scope, scope.global, &body, this);
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

std::string DoWhileLoop::representation() const {
    std::string ret;
    ret.append("do {\n");
    ret.append(body.representation());
    ret.append("\n} while (");
    ret.append(condition->representation());
    ret.append(")");
    return ret;
}
