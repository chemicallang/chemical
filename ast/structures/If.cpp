// Copyright (c) Qinetik 2024.

#include "If.h"
#include "ast/base/BaseType.h"
#include "compiler/SymbolResolver.h"
#include "ast/base/InterpretScope.h"

#ifdef COMPILER_BUILD

#include "compiler/llvmimpl.h"
#include "compiler/Codegen.h"

llvm::Type* IfStatement::llvm_type(Codegen &gen) {
    const auto node = get_value_node();
    return node->llvm_type(gen);
}

llvm::AllocaInst* IfStatement::llvm_allocate(Codegen &gen, const std::string &identifier, BaseType *expected_type) {
    auto allocated = gen.builder->CreateAlloca(expected_type ? expected_type->llvm_type(gen) : llvm_type(gen));
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = allocated;
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return allocated;
}

llvm::Value* IfStatement::llvm_value(Codegen &gen, BaseType *type) {
    code_gen(gen);
    return nullptr;
}

llvm::Value* IfStatement::llvm_assign_value(Codegen &gen, Value *lhs) {
    auto prev_assignable = gen.current_assignable;
    gen.current_assignable = lhs->llvm_pointer(gen);
    code_gen(gen);
    gen.current_assignable = prev_assignable;
    return nullptr;
}

void IfStatement::code_gen(Codegen &gen) {
    code_gen(gen, true);
}

void IfStatement::code_gen(Codegen &gen, bool is_last_block) {

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
        gen.CreateBr(endBlock);
    } else {
        gen.DefaultRet();
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
            gen.CreateBr(endBlock);
        } else {
            gen.DefaultRet();
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
            gen.CreateBr(endBlock);
        } else {
            gen.DefaultRet();
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

#endif

IfStatement::IfStatement(
        std::unique_ptr<Value> condition,
        Scope ifBody,
        std::vector<std::pair<std::unique_ptr<Value>, Scope>> elseIfs,
        std::optional<Scope> elseBody,
        ASTNode* parent_node,
        bool is_value,
        CSTToken* token
) : condition(std::move(condition)), ifBody(std::move(ifBody)),
    elseIfs(std::move(elseIfs)), elseBody(std::move(elseBody)), parent_node(parent_node), is_value(is_value), token(token) {}

void IfStatement::accept(Visitor *visitor) {
    visitor->visit(this);
}

void IfStatement::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>* node_ptr, std::unique_ptr<Value>* value_ptr) {
    linker.scope_start();
    condition->link(linker, condition);
    ifBody.link_sequentially(linker);
    linker.scope_end();
    for(auto& elseIf : elseIfs) {
        linker.scope_start();
        elseIf.first->link(linker, elseIf.first);
        elseIf.second.link_sequentially(linker);
        linker.scope_end();
    }
    if(elseBody.has_value()) {
        linker.scope_start();
        elseBody->link_sequentially(linker);
        linker.scope_end();
    }
}

Value* IfStatement::get_value_node() {
    return Value::get_first_value_from_value_node(this);
}

std::unique_ptr<BaseType> IfStatement::create_type() {
    if(!is_value) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->create_type() : nullptr;
}

std::unique_ptr<BaseType> IfStatement::create_value_type() {
    return create_type();
}

BaseType *IfStatement::known_type() {
    if(!is_value) return nullptr;
    auto last_val = get_value_node();
    return last_val ? last_val->known_type() : nullptr;
}

void IfStatement::interpret(InterpretScope &scope) {
    if (condition->evaluated_bool(scope)) {
        InterpretScope child(&scope, scope.global);
        ifBody.interpret(child);
    } else {
        for (auto const &elseIf: elseIfs) {
            if (elseIf.first->evaluated_bool(scope)) {
                InterpretScope child(&scope, scope.global);
                const_cast<Scope *>(&elseIf.second)->interpret(child);
                return;
            }
        }
        if (elseBody.has_value()) {
            InterpretScope child(&scope, scope.global);
            elseBody->interpret(child);
        }
    }
}