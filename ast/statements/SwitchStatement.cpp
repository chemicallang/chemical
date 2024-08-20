// Copyright (c) Qinetik 2024.

#include "SwitchStatement.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "compiler/SymbolResolver.h"
#include "ast/values/VariantCase.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

void SwitchStatement::code_gen(Codegen &gen, bool last_block) {

    auto total_scopes = defScope.has_value() ? (scopes.size() + 1) : scopes.size();

    // the end block
    llvm::BasicBlock* end = llvm::BasicBlock::Create(*gen.ctx, "end", gen.current_function);

    auto switchInst = gen.builder->CreateSwitch(expression->llvm_value(gen), end, total_scopes);

    bool all_scopes_return = true;

    for(auto& scope : scopes) {
        auto caseBlock = llvm::BasicBlock::Create(*gen.ctx, "case", gen.current_function);
        gen.SetInsertPoint(caseBlock);
        scope.second.code_gen(gen);
        if(!gen.has_current_block_ended) {
            all_scopes_return = false;
        }
        gen.CreateBr(end);

        // TODO check value is of type constant integer (check in analysis phase)
        switchInst->addCase((llvm::ConstantInt*) scope.first->llvm_value(gen), caseBlock);

    }

    bool def_scope_returns = true;

    // default case
    if(defScope.has_value()) {
        auto defCase = llvm::BasicBlock::Create(*gen.ctx, "default", gen.current_function);
        gen.SetInsertPoint(defCase);
        defScope.value().code_gen(gen);
        if(!gen.has_current_block_ended) {
            def_scope_returns = false;
        }
        gen.CreateBr(end);
        switchInst->setDefaultDest(defCase);
    }

    if(end) {
        if (all_scopes_return && def_scope_returns && last_block) {
            end->eraseFromParent();
            gen.destroy_current_scope = false;
            if(!defScope.has_value()) {
                gen.error("A default case must be present when generating switch instruction or it must not be the last statement in the function");
            }
        } else {
            gen.SetInsertPoint(end);
        }
    }

}

void SwitchStatement::code_gen(Codegen &gen, Scope* scope, unsigned int index) {
    code_gen(gen, index == scope->nodes.size() - 1);
}

#endif

SwitchStatement::SwitchStatement(
        std::unique_ptr<Value> expression,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> scopes,
        std::optional<Scope> defScope,
        ASTNode* parent_node
) : expression(std::move(expression)), scopes(std::move(scopes)), defScope(std::move(defScope)), parent_node(parent_node) {

}

void SwitchStatement::declare_and_link(SymbolResolver &linker) {
    expression->link(linker, expression);
    for(auto& scope : scopes) {
        linker.scope_start();
        const auto chain = scope.first->as_access_chain();
        if(chain) {
            const auto first = chain->values[0].get();
            first->link(linker, nullptr, chain->values, 0, nullptr);
            const auto first_linked = first->linked_node();
            if(first_linked && first_linked->as_variant_def()) {
                scope.first = std::unique_ptr<Value>(new VariantCase(std::unique_ptr<AccessChain>((AccessChain*) scope.first.release()), linker));
            }
        }
        scope.first->link(linker, scope.first);
        scope.second.declare_and_link(linker);
        linker.scope_end();
    }
    if(defScope.has_value()) {
        linker.scope_start();
        defScope.value().declare_and_link(linker);
        linker.scope_end();
    }
}

void SwitchStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}