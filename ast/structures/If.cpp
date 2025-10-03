// Copyright (c) Chemical Language Foundation 2025.

#include "If.h"
#include "ast/base/BaseType.h"
#include "ast/types/FunctionType.h"
#include "ast/base/InterpretScope.h"
#include "compiler/ASTDiagnoser.h"
#include "ast/values/IfValue.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type* IfValue::llvm_type(Codegen &gen) {
    const auto node = stmt.get_value_node();
    return node->llvm_type(gen);
}

llvm::Value* IfValue::llvm_value(Codegen& gen, IfStatement& stmt, bool allocate) {

//    auto scope = stmt.get_or_resolve_scope((InterpretScope&) gen.comptime_scope, gen);
//    if(scope.has_value()) {
//        if(scope.value()) {
//            scope.value()->code_gen(gen);
//        }
//        return;
//    } else if(stmt.is_top_level()) {
//        gen.error("top level if statement couldn't be resolved at comptime", &stmt);
//        return;
//    }

    // compare
    llvm::BasicBlock *elseBlock = nullptr;

    // creating a then block
    auto thenBlock = llvm::BasicBlock::Create(*gen.ctx, "ifthen", gen.current_function);

    // creating all the else ifs blocks
    // every else if it has two blocks, one block that checks the condition, the other block which runs when condition succeeds
    const auto total_elseifs = stmt.elseIfs.size();
    std::vector<std::pair<llvm::BasicBlock *, llvm::BasicBlock *>> elseIfsBlocks(total_elseifs);
    unsigned int i = 0;
    while (i < total_elseifs) {
        // else if condition block
        auto conditionBlock = llvm::BasicBlock::Create(*gen.ctx, "elseifcond" + std::to_string(i), gen.current_function);
        // else if body block
        auto elseIfBodyBlock = llvm::BasicBlock::Create(*gen.ctx, "elseifbody" + std::to_string(i), gen.current_function);
        // save
        elseIfsBlocks[i] = std::pair(conditionBlock, elseIfBodyBlock);
        i++;
    }

    // create an else block
    if (stmt.elseBody.has_value()) {
        elseBlock = llvm::BasicBlock::Create(*gen.ctx, "ifelse", gen.current_function);
    }

    // end block
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(*gen.ctx, "ifend", gen.current_function);

    // the block after the first if block
    const auto elseOrEndBlock = elseBlock ? elseBlock : endBlock;
    auto nextBlock = !elseIfsBlocks.empty() ? elseIfsBlocks[0].first : elseOrEndBlock;

    // Branch based on comparison result
    stmt.condition->llvm_conditional_branch(gen, thenBlock, nextBlock);

    // the phi node entries
    std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> incoming;

    // generating then code
    gen.SetInsertPoint(thenBlock);
    const auto ifBodyValue = stmt.ifBody.code_gen_value_scope(gen, allocate, gen.destruct_nodes.size());
    incoming.emplace_back(ifBodyValue, gen.builder->GetInsertBlock());
    bool is_then_returns = gen.has_current_block_ended;
    // TODO send the ending location of the block
    gen.CreateBr(endBlock, stmt.ifBody.encoded_location());

    bool all_elseifs_return = true;

    // generating else if block
    i = 0;
    while (i < total_elseifs) {
        auto &elif = stmt.elseIfs[i];
        auto &pair = elseIfsBlocks[i];

        // generating condition code
        gen.SetInsertPoint(pair.first);
        nextBlock = ((i + 1) < elseIfsBlocks.size()) ? elseIfsBlocks[i + 1].first : elseOrEndBlock;
        elif.first->llvm_conditional_branch(gen, pair.second, nextBlock);

        // generating block code
        gen.SetInsertPoint(pair.second);
        const auto elseIfVal = elif.second.code_gen_value_scope(gen, allocate, gen.destruct_nodes.size());
        incoming.emplace_back(elseIfVal, gen.builder->GetInsertBlock());
        if(!gen.has_current_block_ended) {
            all_elseifs_return = false;
        }
        // TODO send the ending location of the block
        gen.CreateBr(endBlock, elif.second.encoded_location());
        i++;
    }

    // generating else block
    bool is_else_returns = false;
    if (elseBlock) {
        gen.SetInsertPoint(elseBlock);
        const auto elseBlockVal = stmt.elseBody.value().code_gen_value_scope(gen, allocate, gen.destruct_nodes.size());
        incoming.emplace_back(elseBlockVal, gen.builder->GetInsertBlock());
        is_else_returns = gen.has_current_block_ended;
        // TODO send the ending location of the block
        gen.CreateBr(endBlock, stmt.elseBody->encoded_location());
    }

//    if(endBlock) {
//        // set to end block
//        if (is_then_returns && all_elseifs_return && is_else_returns) {
//            endBlock->eraseFromParent();
//            gen.destroy_current_scope = false;
//        }
//    }

    gen.SetInsertPoint(endBlock);

    const auto phi = gen.builder->CreatePHI(ifBodyValue->getType(), total_elseifs + 1 + (elseBlock ? 1 : 0));

    for(auto& inc : incoming) {
        phi->addIncoming(gen.implicit_cast(inc.first, stmt.known_type(),ifBodyValue->getType()), inc.second);
    }

    return phi;

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
    // TODO send the ending location of the block
    gen.CreateBr(endBlock, ifBody.encoded_location());

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
        // TODO send the ending location of the block
        gen.CreateBr(endBlock, elif.second.encoded_location());
        i++;
    }

    // generating else block
    bool is_else_returns = false;
    if (elseBlock) {
        gen.SetInsertPoint(elseBlock);
        elseBody.value().code_gen(gen);
        is_else_returns = gen.has_current_block_ended;
        // TODO send the ending location of the block
        gen.CreateBr(endBlock, elseBody->encoded_location());
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

bool IfValue::add_child_index(Codegen& gen, std::vector<llvm::Value *>& indexes, const chem::string_view& name) {
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
    if(condition_value) {
        return &ifBody;
    } else {
        for(auto& elseIf : elseIfs) {
            auto constant = elseIf.first->evaluated_value(scope);
            if(!constant || constant->val_kind() != ValueKind::Bool) {
                gen->error("couldn't get constant value for top level if statement's condition", (ASTNode*) this);
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

std::optional<Scope*> IfStatement::resolve_evaluated_scope(InterpretScope& comptime_scope, ASTDiagnoser& diagnoser) {
    auto condition_val = get_condition_const(comptime_scope);
    if(condition_val.has_value()) {
        return get_evaluated_scope(comptime_scope, &diagnoser, condition_val.value());
    } else {
        return std::nullopt;
    }
}

Value* IfStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

BaseType *IfStatement::known_type() {
    auto last_val = get_value_node();
    return last_val ? last_val->getType() : nullptr;
}

ASTNode *IfValue::linked_node() {
    const auto known = stmt.known_type();
    return known ? known->linked_node() : nullptr;
}