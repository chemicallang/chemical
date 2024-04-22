// Copyright (c) Qinetik 2024.

#include "SwitchStatement.h"
#include "ast/base/Value.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void SwitchStatement::code_gen(Codegen &gen, bool last_block) {

    auto total_scopes = defScope.has_value() ? (scopes.size() + 1) : scopes.size();

    // the end block
    llvm::BasicBlock* end = nullptr;
    auto has_end = !(last_block && defScope.has_value());
    if(has_end) {
        end = llvm::BasicBlock::Create(*gen.ctx, "end", gen.current_function);
    }

    auto switchInst = gen.builder->CreateSwitch(expression->llvm_value(gen), end, total_scopes);

    for(auto& scope : scopes) {
        auto caseBlock = llvm::BasicBlock::Create(*gen.ctx, "case", gen.current_function);
        gen.SetInsertPoint(caseBlock);
        scope.second.code_gen(gen);
        gen.CreateBr(end);

        // TODO check value is of type constant integer
        switchInst->addCase((llvm::ConstantInt*) scope.first->llvm_value(gen), caseBlock);

    }

    // default case
    if(defScope.has_value()) {
        auto defCase = llvm::BasicBlock::Create(*gen.ctx, "default", gen.current_function);
        gen.SetInsertPoint(defCase);
        defScope.value().code_gen(gen);
        gen.CreateBr(end);
        switchInst->setDefaultDest(defCase);
    }

    if(end) {
        gen.SetInsertPoint(end);
    }

}

void SwitchStatement::code_gen(Codegen &gen, std::vector<std::unique_ptr<ASTNode>> &nodes, unsigned int index) {
    code_gen(gen, index == nodes.size() - 1);
}

#endif

SwitchStatement::SwitchStatement(
        std::unique_ptr<Value> expression,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes,
        std::optional<Scope> defScope
) : expression(std::move(expression)), scopes(std::move(scopes)), defScope(std::move(defScope)) {

}

void SwitchStatement::declare_and_link(SymbolResolver &linker) {
    expression->link(linker);
}

void SwitchStatement::accept(Visitor &visitor) {
    visitor.visit(this);
}

std::string SwitchStatement::representation() const {
    std::string rep("switch(" + expression->representation() +  ") {\n");
    for(auto& caseVal : scopes) {
        rep.append("case ");
        rep.append(caseVal.first->representation());
        rep.append(":\n");
        rep.append(caseVal.second.representation());
        rep.append("\n");
    }
    if(defScope.has_value()) {
        rep.append("default:\n");
        rep.append(defScope.value().representation());
        rep.append("\n");
    }
    rep.append("}");
    return rep;
}