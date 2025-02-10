// Copyright (c) Qinetik 2024.

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
    gen.CreateBr(condBlock);

    // condition block
    gen.SetInsertPoint(condBlock);
    auto comparison = conditionExpr->llvm_value(gen);
    gen.CreateCondBr(comparison, thenBlock, endBlock);

    // then block
    gen.SetInsertPoint(thenBlock);
    gen.loop_body_gen(body, condBlock, endBlock);
    incrementerExpr->code_gen(gen);
    gen.CreateBr(condBlock);

    // end block
    gen.SetInsertPoint(endBlock);

}

#endif

/**
 * @brief Construct a new ForLoop object with an empty body
 */
//ForLoop::ForLoop(
//        std::unique_ptr<VarInitStatement> initializer,
//        std::unique_ptr<Value> conditionExpr,
//        std::unique_ptr<ASTNode> incrementerExpr,
//        ASTNode* parent_node,
//        CSTToken* token
//) : initializer(std::move(initializer)),
//    conditionExpr(std::move(conditionExpr)),
//    incrementerExpr(std::move(incrementerExpr)),
//    parent_node(parent_node), token(token) {
//
//}

void ForLoop::accept(Visitor *visitor) {
    visitor->visit(this);
}

void ForLoop::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    linker.scope_start();
    initializer->declare_and_link(linker, (ASTNode*&) initializer);
    conditionExpr->link(linker, conditionExpr);
    incrementerExpr->declare_and_link(linker, incrementerExpr);
    body.link_sequentially(linker);
    linker.scope_end();
}

void ForLoop::interpret(InterpretScope &scope) {
    InterpretScope child(&scope, scope.allocator, scope.global);
    initializer->interpret(child);
    while (conditionExpr->evaluated_bool(child)) {
        body.interpret(child);
        if (stoppedInterpretation) {
            stoppedInterpretation = false;
            break;
        }
        incrementerExpr->interpret(child);
    }
}

void ForLoop::stopInterpretation() {
    stoppedInterpretation = true;
}