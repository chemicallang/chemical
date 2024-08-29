// Copyright (c) Qinetik 2024.

#include "Scope.h"
#include <iostream>
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/LoopBlock.h"
#include "ast/base/BaseType.h"
#include "ast/statements/Break.h"

void Scope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->interpret(scope);
    }
}

Scope::Scope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node) : nodes(std::move(nodes)), parent_node(parent_node) {

}

void Scope::accept(Visitor *visitor) {
    visitor->visit(this);
}

#ifdef DEBUG
void Scope::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    throw std::runtime_error("Scope::declare_top_level shouldn't be called, other link methods should be called");
}
void Scope::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    throw std::runtime_error("Scope::declare_and_link shouldn't be called, other link methods should be called");
}
#endif

void Scope::link_sequentially(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_top_level(linker, node);
        node->declare_and_link(linker, node);
    }
}

void Scope::link_asynchronously(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_top_level(linker, node);
    }
    for (auto &node: nodes) {
        node->declare_and_link(linker, node);
    }
}

void Scope::stopInterpretOnce() {

}

void LoopBlock::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) {
    body.link_sequentially(linker);
}

Value* get_first_broken(LoopASTNode* loop_node) {
    for(auto& node : loop_node->body.nodes) {
        const auto k = node->kind();
        if(k == ASTNodeKind::BreakStmt) {
            return node->as_break_stmt_unsafe()->value.get();
        } else if(ASTNode::isLoopASTNode(k)) {
            auto down = get_first_broken(node->as_loop_node_unsafe());
            if(down) {
                return down;
            }
        }
    }
    return nullptr;
}

Value* LoopBlock::get_first_broken() {
    return ::get_first_broken(this);
}

std::unique_ptr<BaseType> LoopBlock::create_value_type() {
    return get_first_broken()->create_type();
}

std::unique_ptr<BaseType> LoopBlock::create_type() {
    return get_first_broken()->create_type();
}

hybrid_ptr<BaseType> LoopBlock::get_base_type() {
    return get_first_broken()->get_base_type();
}

hybrid_ptr<BaseType> LoopBlock::get_value_type() {
    return get_first_broken()->get_base_type();
}

BaseType* LoopBlock::known_type() {
    return get_first_broken()->known_type();
}
