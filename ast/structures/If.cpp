// Copyright (c) Chemical Language Foundation 2025.

#include "If.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ast/types/FunctionType.h"
#include "ast/base/InterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type* IfStatement::llvm_type(Codegen &gen) {
    const auto node = get_value_node();
    return node->llvm_type(gen);
}

llvm::AllocaInst* IfStatement::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    const auto allocated = gen.builder->CreateAlloca(expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
    gen.di.instr(allocated, Value::encoded_location());
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { nullptr, allocated };
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

llvm::Value* IfStatement::llvm_value(Codegen &gen, BaseType *type) {
    code_gen(gen);
    return nullptr;
}

void IfStatement::llvm_assign_value(Codegen &gen, llvm::Value *lhsPtr, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = { lhs, lhsPtr };
    code_gen(gen);
    gen.current_assignable = prev_assignable;
}

void IfStatement::code_gen(Codegen &gen) {
    code_gen(gen, true);
}

void IfStatement::code_gen_external_declare(Codegen &gen) {

    auto scope = get_or_resolve_scope((InterpretScope&) gen.comptime_scope, gen);
    if(scope.has_value()) {
        if(scope.value()) {
            scope.value()->external_declare_top_level(gen);
        }
        return;
    } else {
        gen.error("top level if statement couldn't be evaluated at comptime", (ASTNode*) this);
    }

}

void IfStatement::code_gen_declare(Codegen &gen) {

    auto scope = get_or_resolve_scope((InterpretScope&) gen.comptime_scope, gen);
    if(scope.has_value()) {
        if(scope.value()) {
            scope.value()->gen_declare_top_level(gen);
        }
        return;
    }

}

void IfStatement::code_gen(Codegen &gen, bool is_last_block) {

    auto scope = get_or_resolve_scope((InterpretScope&) gen.comptime_scope, gen);
    if(scope.has_value()) {
        if(scope.value()) {
            scope.value()->code_gen(gen);
        }
        return;
    } else if(is_top_level()) {
        gen.error("top level if statement couldn't be resolved at comptime", (ASTNode*) this);
        return;
    }

    // compare
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
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(*gen.ctx, "ifend", gen.current_function);

    // the block after the first if block
    const auto elseOrEndBlock = elseBlock ? elseBlock : endBlock;
    auto nextBlock = !elseIfsBlocks.empty() ? elseIfsBlocks[0].first : elseOrEndBlock;

    // Branch based on comparison result
    condition->llvm_conditional_branch(gen, thenBlock, nextBlock);

    // generating then code
    gen.SetInsertPoint(thenBlock);
    ifBody.code_gen(gen);
    bool is_then_returns = gen.has_current_block_ended;
    if(endBlock) {
        // TODO send the ending location of the block
        gen.CreateBr(endBlock, ifBody.encoded_location());
    } else {
        // TODO send the ending location here
        gen.DefaultRet(ifBody.encoded_location());
    }

    bool all_elseifs_return = true;
    // generating else if block
    i = 0;
    while (i < elseIfsBlocks.size()) {
        auto &elif = elseIfs[i];
        auto &pair = elseIfsBlocks[i];

        // generating condition code
        gen.SetInsertPoint(pair.first);
        nextBlock = ((i + 1) < elseIfsBlocks.size()) ? elseIfsBlocks[i + 1].first : elseOrEndBlock;
        elif.first->llvm_conditional_branch(gen, pair.second, nextBlock);

        // generating block code
        gen.SetInsertPoint(pair.second);
        elif.second.code_gen(gen);
        if(!gen.has_current_block_ended) {
            all_elseifs_return = false;
        }
        if(endBlock) {
            // TODO send the ending location of the block
            gen.CreateBr(endBlock, elif.second.encoded_location());
        } else {
            // TODO send the ending location here
            gen.DefaultRet(elif.second.encoded_location());
        }
        i++;
    }

    // generating else block
    bool is_else_returns = false;
    if (elseBlock) {
        gen.SetInsertPoint(elseBlock);
        elseBody.value().code_gen(gen);
        is_else_returns = gen.has_current_block_ended;
        if(endBlock) {
            // TODO send the ending location of the block
            gen.CreateBr(endBlock, elseBody->encoded_location());
        } else {
            // TODO send the ending location of the block
            gen.DefaultRet(elseBody->encoded_location());
        }
    }

    if(endBlock) {
        // set to end block
        if (is_then_returns && all_elseifs_return && is_else_returns) {
            endBlock->eraseFromParent();
            gen.destroy_current_scope = false;
        } else {
            gen.SetInsertPoint(endBlock);
        }
    }

}

void IfStatement::code_gen(Codegen &gen, Scope* scope, unsigned int index) {
    code_gen(gen, index != scope->nodes.size() - 1);
}

bool IfStatement::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
    const auto linked = linked_node();
    return linked != nullptr && linked->add_child_index(gen, indexes, name);
}

#endif

std::optional<bool> IfStatement::get_condition_const(InterpretScope& scope) {
    auto constant = condition->evaluated_value(scope);
    if(!constant || constant->val_kind() != ValueKind::Bool) {
        return std::nullopt;
    }
    return constant->get_the_bool();
}

Scope* IfStatement::get_evaluated_scope(InterpretScope& scope, ASTDiagnoser* gen, bool condition_value) {
    auto err = "couldn't get constant value for top level if statement's condition";
    if(condition_value) {
        return &ifBody;
    } else {
        for(auto& elseIf : elseIfs) {
            auto constant = elseIf.first->evaluated_value(scope);
            if(!constant || constant->val_kind() != ValueKind::Bool) {
                gen->error(err, (ASTNode*) this);
                return nullptr;
            }
            if(constant->get_the_bool()) {
                return &elseIf.second;
            }
        }
        if(elseBody.has_value()) {
            return &elseBody.value();
        }
    };
    return nullptr;
}

bool IfStatement::compile_time_computable() {
    if(!condition->compile_time_computable()) {
        return false;
    }
    for(auto& elseIf : elseIfs) {
        if(!elseIf.first->compile_time_computable()) {
            return false;
        }
    }
    return true;
}

bool IfStatement::link_conditions(SymbolResolver &linker) {
    if(!condition->link(linker, condition)) {
        return false;
    }
    for (auto& cond: elseIfs) {
        if(!cond.first->link(linker, cond.first)) {
            return false;
        }
    }
    return true;
}

std::optional<Scope*> IfStatement::resolve_evaluated_scope(InterpretScope& comptime_scope, ASTDiagnoser& diagnoser) {
    auto condition_val = resolved_condition ? get_condition_const(comptime_scope) : std::optional(false);
    if(condition_val.has_value()) {
        auto eval = get_evaluated_scope(comptime_scope, &diagnoser, condition_val.value());
        computed_scope = eval;
        return eval;
    } else {
        is_computable = false;
        return std::nullopt;
    }
}

Scope* IfStatement::link_evaluated_scope(SymbolResolver& linker) {
    is_computable = true;
    if(!link_conditions(linker)) {
        resolved_condition = false;
    }
    auto condition_val = resolved_condition ? get_condition_const((InterpretScope&) linker.comptime_scope) : std::optional(false);
    if(condition_val.has_value()) {
        auto eval = get_evaluated_scope((InterpretScope&) linker.comptime_scope, &linker, condition_val.value());
        computed_scope = eval;
        return eval;
    } else {
        is_computable = false;
    }
    return nullptr;
}

void IfStatement::declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) {
    if(is_top_level()) {
        auto scope = link_evaluated_scope(linker);
        if(scope) {
            scope->declare_top_level(linker, (ASTNode*&) computed_scope.value());
        }
    }
}

void IfStatement::link_signature(SymbolResolver &linker) {
    if(is_top_level()) {
        auto scope = get_evaluated_scope_by_linking(linker);
        if(scope) {
            scope->link_signature(linker);
        }
    }
}

void IfStatement::declare_and_link(SymbolResolver &linker, Value** value_ptr) {
    if(!is_computable) {
        link_conditions(linker);
    }
    if(is_computable || compile_time_computable()) {
        is_computable = true;
        if(computed_scope.has_value()) {
            auto scope = computed_scope.value();
            if(scope) {
                scope->declare_and_link(linker, (ASTNode*&) computed_scope.value());
            }
            return;
        }
        if(!linker.comptime_context) {
            auto condition_val = resolved_condition ? get_condition_const((InterpretScope&) linker.comptime_scope) : std::optional(false);
            if (condition_val.has_value()) {
                auto eval = get_evaluated_scope((InterpretScope&) linker.comptime_scope, &linker, condition_val.value());
                computed_scope = eval;
                if (eval) {
                    eval->declare_and_link(linker, (ASTNode*&) computed_scope.value());
                }
                return;
            } else {
                is_computable = false;
            }
        }
    }
    if(is_top_level()) {
        linker.error("cannot evaluate compile time top level if statement", (ASTNode*) this);
        return;
    }

    if(!linker.current_func_type) return;

    // temporary moved identifiers and chains
    std::vector<VariableIdentifier*> moved_ids;
    std::vector<AccessChain*> moved_chains;

    // link the body
    linker.scope_start();
    linker.link_body_seq_backing_moves(ifBody, moved_ids, moved_chains);
    linker.scope_end();
    // link the else ifs
    for(auto& elseIf : elseIfs) {
        linker.scope_start();
        linker.link_body_seq_backing_moves(elseIf.second, moved_ids, moved_chains);
        linker.scope_end();
    }
    // link the else body
    if(elseBody.has_value()) {
        linker.scope_start();
        linker.link_body_seq_backing_moves(elseBody.value(), moved_ids, moved_chains);
        linker.scope_end();
    }

    const auto curr_func = linker.current_func_type;
    curr_func->restore_moved_ids(moved_ids);
    curr_func->restore_moved_chains(moved_chains);
}

Value* IfStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

BaseType* IfStatement::create_type(ASTAllocator& allocator) {
    if(!is_value) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->create_type(allocator) : nullptr;
}

BaseType *IfStatement::known_type() {
    if(!is_value) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->known_type() : nullptr;
}

ASTNode *IfStatement::linked_node() {
    const auto known = known_type();
    return known ? known->linked_node() : nullptr;
}