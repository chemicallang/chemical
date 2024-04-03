// Copyright (c) Qinetik 2024.

#include "SwitchStatement.h"
#include "ast/base/Value.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void SwitchStatement::code_gen(Codegen &gen) {

    auto total_scopes = defScope.has_value() ? (scopes.size() + 1) : scopes.size();
    auto switchInst = gen.builder->CreateSwitch(expression->llvm_value(gen), nullptr, total_scopes);

    for(auto& scope : scopes) {
        auto caseBlock = llvm::BasicBlock::Create(*gen.ctx, "case", gen.current_function);
        gen.SetInsertPoint(caseBlock);
        scope.second.code_gen(gen);

        // TODO check value is of type constant integer
        switchInst->addCase((llvm::ConstantInt*) scope.first->llvm_value(gen), caseBlock);

    }

    // default case
    if(defScope.has_value()) {
        auto defCase = llvm::BasicBlock::Create(*gen.ctx, "default", gen.current_function);
        gen.SetInsertPoint(defCase);
        defScope.value().code_gen(gen);
        switchInst->setDefaultDest(defCase);
    }

    auto end = llvm::BasicBlock::Create(*gen.ctx, "end", gen.current_function);
    gen.SetInsertPoint(end);

}

#endif

SwitchStatement::SwitchStatement(
        std::unique_ptr<Value> expression,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes,
        std::optional<Scope> defScope
) : expression(std::move(expression)), scopes(std::move(scopes)), defScope(std::move(defScope)) {

}

void SwitchStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string SwitchStatement::representation() const {
    std::string rep("switch {\n");

    rep.append("\n}");
    return rep;
}