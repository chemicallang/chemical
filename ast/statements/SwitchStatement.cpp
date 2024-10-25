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
    gen.current_assignable = { nullptr, allocated };
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

llvm::Value* SwitchStatement::llvm_value(Codegen &gen, BaseType *type) {
    code_gen(gen);
    return nullptr;
}

llvm::Value * SwitchStatement::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { lhs, lhsPtr };
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return nullptr;
}

void SwitchStatement::code_gen(Codegen &gen, bool last_block) {

    auto total_scopes = scopes.size();

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
                if (scopes.size() == variant_def->variables.size() && !has_default_case()) {
                    // TODO only do this when switch is a value
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

    unsigned scope_ind = 0;
    const auto scopes_size = scopes.size();
    while(scope_ind < scopes_size) {
        auto& scope = scopes[scope_ind];

        caseBlock = llvm::BasicBlock::Create(*gen.ctx, "case", gen.current_function);
        gen.SetInsertPoint(caseBlock);
        scope.code_gen(gen);
        if(!gen.has_current_block_ended) {
            all_scopes_return = false;
        }
        gen.CreateBr(end);

        // TODO check value is constant (check in analysis phase)
        for(auto& switch_case : cases) {
            if(switch_case.second == scope_ind) {
                switchInst->addCase((llvm::ConstantInt*) switch_case.first->llvm_value(gen), caseBlock);
            }
        }

        if(defScopeInd == scope_ind) {
            switchInst->setDefaultDest(caseBlock);
        }

        scope_ind++;
    }

    if(end) {
        if (all_scopes_return && last_block) {
            end->eraseFromParent();
            gen.destroy_current_scope = false;
            if(!has_default_case()) {
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
        Value* expression,
        ASTNode* parent_node,
        bool is_value,
        SourceLocation location
) : expression(expression), parent_node(parent_node), is_value(is_value), location(location) {

}

Value* SwitchStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

BaseType* SwitchStatement::create_type(ASTAllocator& allocator) {
    if(!is_value || scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->create_type(allocator) : nullptr;
}

BaseType* SwitchStatement::create_value_type(ASTAllocator& allocator) {
    return create_type(allocator);
}

BaseType *SwitchStatement::known_type() {
    if(!is_value || scopes.empty()) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->known_type() : nullptr;
}

ASTNode *SwitchStatement::linked_node() {
    const auto known = known_type();
    return known ? known->linked_node() : nullptr;
}

bool SwitchStatement::declare_and_link(SymbolResolver &linker, Value** value_ptr) {
    VariantDefinition* variant_def = nullptr;
    auto& astAlloc = *linker.ast_allocator;
    bool result = true;
    if(expression->link(linker, expression)) {
        const auto expr_type = expression->known_type();
        if(expr_type) {
        const auto linked = expr_type->linked_node();
            if(linked) {
                variant_def = linked->as_variant_def();
                if (value_ptr && variant_def && (scopes.size() < variant_def->variables.size() && !has_default_case())) {
                    linker.error("expected all cases of variant in switch statement when no default case is specified", (ASTNode*) this);
                    return false;
                }
            }
        }
    } else {
        result = false;
    }
    if(variant_def) {
        for (auto& switch_case: cases) {
            // replace variant case access chains in switch cases
            const auto chain = switch_case.first->as_access_chain();
            if (chain) {
                switch_case.first = new (astAlloc.allocate<VariantCase>()) VariantCase(chain, linker, this, chain->location);
            }
        }
    }
    unsigned i = 0;
    const auto scopes_size = scopes.size();
    while(i < scopes_size) {
        auto& scope = scopes[i];
        linker.scope_start();
        for(auto& switch_case : cases) {
            if(switch_case.second == i && switch_case.first) {
                // link the switch case value
                switch_case.first->link(linker, switch_case.first);
            }
        }
        scope.link_sequentially(linker);
        linker.scope_end();
        i++;
    }
    if(result && value_ptr) {
        auto val_node = get_value_node();
        if(!val_node) {
            linker.error("expected a single value node for the value", (ASTNode*) this);
            return false;
        }
    }
    return result;
}

void SwitchStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}