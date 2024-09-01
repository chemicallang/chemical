// Copyright (c) Qinetik 2024.

#include "SwitchStatement.h"
#include "ast/base/Value.h"
#include "ast/values/AccessChain.h"
#include "compiler/SymbolResolver.h"
#include "ast/values/VariantCase.h"
#include "ast/structures/VariantDefinition.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type* SwitchStatement::llvm_type(Codegen &gen) {
    const auto node = get_value_node();
    return node->llvm_type(gen);
}

llvm::AllocaInst* SwitchStatement::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    auto allocated = gen.builder->CreateAlloca(expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = allocated;
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

llvm::Value* SwitchStatement::llvm_value(Codegen &gen, BaseType *type) {
    code_gen(gen);
    return nullptr;
}

llvm::Value* SwitchStatement::llvm_assign_value(Codegen &gen, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = lhs->llvm_pointer(gen);
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return nullptr;
}

void SwitchStatement::code_gen(Codegen &gen, bool last_block) {

    auto total_scopes = defScope.has_value() ? (scopes.size() + 1) : scopes.size();

    // the end block
    llvm::BasicBlock* end = llvm::BasicBlock::Create(*gen.ctx, "end", gen.current_function);

    // this boolean can be set to true, to set to last case as default
    // this should be only set when it's guaranteed that default scope is not needed
    // because all cases are covered
    bool auto_default_case = false;

    llvm::Value* expr_value = expression->llvm_value(gen);
    const auto expr_type = expression->known_type();
    if(expr_type) {
        const auto linked = expr_type->linked_node();
        if(linked) {
            auto variant_def = linked->as_variant_def();
            if (variant_def) {
                if (scopes.size() == variant_def->variables.size() && !defScope.has_value()) {
                    auto_default_case = true;
                }
                const auto def_type = variant_def->llvm_type(gen);
                std::vector<llvm::Value*> idxList { gen.builder->getInt32(0), gen.builder->getInt32(0) };
                const auto gep = gen.builder->CreateGEP(def_type, expr_value, idxList, "",gen.inbounds);
                expr_value = gen.builder->CreateLoad(gen.builder->getInt32Ty(), gep, "");

            }
        }
    }

    auto switchInst = gen.builder->CreateSwitch(expr_value, end, total_scopes);

    bool all_scopes_return = true;

    llvm::BasicBlock* caseBlock = nullptr;

    for(auto& scope : scopes) {
        caseBlock = llvm::BasicBlock::Create(*gen.ctx, "case", gen.current_function);
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
                if(auto_default_case && caseBlock) {
                    switchInst->setDefaultDest(caseBlock);
                } else {
                    gen.error(
                            "A default case must be present when generating switch instruction or it must not be the last statement in the function", (ASTNode*) this);
                }
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
        ASTNode* parent_node,
        bool is_value,
        CSTToken* token
) : expression(std::move(expression)), scopes(std::move(scopes)), defScope(std::move(defScope)), parent_node(parent_node), is_value(is_value), token(token) {

}

Value* SwitchStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

std::unique_ptr<BaseType> SwitchStatement::create_type() {
    if(!is_value || scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->create_type() : nullptr;
}

std::unique_ptr<BaseType> SwitchStatement::create_value_type() {
    return create_type();
}

BaseType *SwitchStatement::known_type() {
    if(!is_value || scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->known_type() : nullptr;
}

void SwitchStatement::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>* node_ptr, std::unique_ptr<Value>* value_ptr) {
    expression->link(linker, expression);
    VariantDefinition* variant_def = nullptr;
    const auto linked = expression->known_type()->linked_node();
    if(linked) {
        variant_def = linked->as_variant_def();
        if (variant_def && (scopes.size() < variant_def->variables.size() && !defScope.has_value())) {
            linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) this);
            return;
        }
    }
    for(auto& scope : scopes) {
        linker.scope_start();
        if(variant_def) {
            const auto chain = scope.first->as_access_chain();
            if (chain) {
                scope.first = std::unique_ptr<Value>(new VariantCase(std::unique_ptr<AccessChain>((AccessChain*) scope.first.release()), linker, this, nullptr));
            }
        }
        scope.first->link(linker, scope.first);
        scope.second.link_sequentially(linker);
        linker.scope_end();
    }
    if(defScope.has_value()) {
        linker.scope_start();
        defScope.value().link_sequentially(linker);
        linker.scope_end();
    }
}

void SwitchStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}