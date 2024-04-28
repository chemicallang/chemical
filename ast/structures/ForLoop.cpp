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

/**
 * @brief Construct a new ForLoop object with an empty body
 */
ForLoop::ForLoop(
        std::unique_ptr<VarInitStatement> initializer,
        std::unique_ptr<Value> conditionExpr,
        std::unique_ptr<ASTNode> incrementerExpr
) : initializer(std::move(initializer)),
    conditionExpr(std::move(conditionExpr)), incrementerExpr(std::move(incrementerExpr)) {}

/**
 * @brief Construct a new ForLoop object.
 */
ForLoop::ForLoop(
        std::unique_ptr<VarInitStatement> initializer,
        std::unique_ptr<Value> conditionExpr,
        std::unique_ptr<ASTNode> incrementerExpr,
        LoopScope body
) : initializer(std::move(initializer)),
    conditionExpr(std::move(conditionExpr)), incrementerExpr(std::move(incrementerExpr)),
    LoopASTNode(std::move(body)) {}

void ForLoop::accept(Visitor &visitor) {
    visitor.visit(this);
}

void ForLoop::declare_and_link(SymbolResolver &linker) {
    linker.scope_start();
    initializer->declare_and_link(linker);
    conditionExpr->link(linker);
    incrementerExpr->declare_and_link(linker);
    body.declare_and_link(linker);
    linker.scope_end();
}

void ForLoop::interpret(InterpretScope &scope) {
    InterpretScope child(&scope, scope.global, &body, this);
    initializer->interpret(child);
    while (conditionExpr->evaluated_bool(child)) {
        body.interpret(child);
        if (stoppedInterpretation) {
            stoppedInterpretation = false;
            break;
        }
        incrementerExpr->interpret(child);
    }
    initializer->interpret_scope_ends(child);
}

void ForLoop::stopInterpretation() {
    stoppedInterpretation = true;
}

std::string ForLoop::representation() const {
    std::string ret("for(");
    ret.append(initializer->representation());
    ret.append(1, ';');
    ret.append(conditionExpr->representation());
    ret.append(1, ';');
    ret.append(incrementerExpr->representation());
    ret.append("){\n");
    ret.append(body.representation());
    ret.append("\n}");
    return ret;
}