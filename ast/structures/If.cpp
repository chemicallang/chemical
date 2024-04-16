// Copyright (c) Qinetik 2024.

#include "If.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void IfStatement::code_gen(Codegen &gen) {

    // compare
    auto comparison = condition->llvm_value(gen);

    llvm::BasicBlock *elseBlock = nullptr;

    // creating a then block
    auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "ifthen", gen.current_function);

    // creating all the else ifs blocks
    // every else if it has two blocks, one block that checks the condition, the other block which runs when condition succeeds
    std::vector<std::pair<llvm::BasicBlock *, llvm::BasicBlock *>> elseIfsBlocks(elseIfs.size());
    unsigned int i = 0;
    while (i < elseIfs.size()) {
        // else if condition block
        auto conditionBlock = llvm::BasicBlock::Create(*gen.ctx, "elseifcond" + std::to_string(i),
                                                       gen.current_function);
        // else if body block
        auto elseIfBodyBlock = llvm::BasicBlock::Create(*gen.ctx, "elseifbody" + std::to_string(i),
                                                        gen.current_function);
        // save
        elseIfsBlocks[i] = std::pair(conditionBlock, elseIfBodyBlock);
        i++;
    }

    // create an else block
    if (elseBody.has_value()) {
        elseBlock = llvm::BasicBlock::Create(*gen.ctx, "ifelse", gen.current_function);
    }

    // end block
    auto endBlock = llvm::BasicBlock::Create(*gen.ctx, "ifend", gen.current_function);

    // the block after the first if block
    const auto elseOrEndBlock = elseBlock ? elseBlock : endBlock;
    auto nextBlock = !elseIfsBlocks.empty() ? elseIfsBlocks[0].first : elseOrEndBlock;

    // Branch based on comparison result
    gen.CreateCondBr(comparison, thenBlock, nextBlock);

    // generating then code
    gen.SetInsertPoint(thenBlock);
    ifBody.code_gen(gen);
    gen.CreateBr(endBlock);

    // generating else if block
    i = 0;
    while (i < elseIfsBlocks.size()) {
        auto &elif = elseIfs[i];
        auto &pair = elseIfsBlocks[i];

        // generating condition code
        gen.SetInsertPoint(pair.first);
        comparison = elif.first->llvm_value(gen);
        nextBlock = ((i + 1) < elseIfsBlocks.size()) ? elseIfsBlocks[i + 1].first : elseOrEndBlock;
        gen.CreateCondBr(comparison, pair.second, nextBlock);

        // generating block code
        gen.SetInsertPoint(pair.second);
        elif.second.code_gen(gen);
        gen.CreateBr(endBlock);

        i++;
    }

    // generating else block
    if (elseBlock) {
        gen.SetInsertPoint(elseBlock);
        elseBody.value().code_gen(gen);
        gen.CreateBr(endBlock);
    }

    // set to end block
    gen.SetInsertPoint(endBlock);

}

#endif

IfStatement::IfStatement(
        std::unique_ptr<Value> condition,
        Scope ifBody,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs,
        std::optional<Scope> elseBody
) : condition(std::move(condition)), ifBody(std::move(ifBody)),
    elseIfs(std::move(elseIfs)), elseBody(std::move(elseBody)) {}

void IfStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

void IfStatement::declare_and_link(SymbolResolver &linker) {
    condition->link(linker);
    ifBody.declare_and_link(linker);
    ifBody.undeclare_on_scope_end(linker);
    for(auto& elseIf : elseIfs) {
        elseIf.first->link(linker);
        elseIf.second.declare_and_link(linker);
        elseIf.second.undeclare_on_scope_end(linker);
    }
    if(elseBody.has_value()) {
        elseBody->declare_and_link(linker);
        elseBody->undeclare_on_scope_end(linker);
    }
}

void IfStatement::interpret(InterpretScope &scope) {
    if (condition->evaluated_bool(scope)) {
        InterpretScope child(&scope, scope.global, &ifBody, this);
        ifBody.interpret(child);
    } else {
        for (auto const &elseIf: elseIfs) {
            if (elseIf.first->evaluated_bool(scope)) {
                InterpretScope child(&scope, scope.global, const_cast<Scope *>(&elseIf.second), this);
                const_cast<Scope *>(&elseIf.second)->interpret(child);
                return;
            }
        }
        if (elseBody.has_value()) {
            InterpretScope child(&scope, scope.global, &elseBody.value(), this);
            elseBody->interpret(child);
        }
    }
}

std::string IfStatement::representation() const {
    std::string rep;
    rep.append("if(");
    rep.append(condition->representation());
    rep.append("){\n");
    rep.append(ifBody.representation());
    rep.append("\n}");
    int i = 0;
    while (i < elseIfs.size()) {
        rep.append("else if(");
        rep.append(elseIfs[i].first->representation());
        rep.append(") {\n");
        rep.append(elseIfs[i].second.representation());
        rep.append("\n}");
        i++;
    }
    if (elseBody.has_value()) {
        rep.append("else {\n");
        rep.append(elseBody.value().representation());
        rep.append("\n}");
    }
    return rep;
}