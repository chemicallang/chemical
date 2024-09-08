// Copyright (c) Qinetik 2024.

#include "Scope.h"
#include <iostream>
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/LoopBlock.h"
#include "ast/structures/If.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/base/BaseType.h"
#include "ast/statements/Break.h"
#include "compiler/SymbolResolver.h"

void Scope::interpret(InterpretScope &scope) {
    for (const auto &node: nodes) {
        node->interpret(scope);
    }
}

Scope::Scope(std::vector<std::unique_ptr<ASTNode>> nodes, ASTNode* parent_node, CSTToken* token) : nodes(std::move(nodes)), parent_node(parent_node), token(token) {

}

void Scope::accept(Visitor *visitor) {
    visitor->visit(this);
}

void Scope::declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    for (auto &node: nodes) {
        node->declare_top_level(linker, node);
    }
}

void Scope::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) {
    for (auto &node: nodes) {
        node->declare_and_link(linker, node);
    }
}

void Scope::link_sequentially(SymbolResolver &linker) {
    for(auto& node : nodes) {
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

bool LoopBlock::link(SymbolResolver &linker, std::unique_ptr<Value> &value_ptr) {
    body.link_sequentially(linker);
    return true;
}

Value* get_first_broken(Scope* body) {
    Value* value = nullptr;
    for(auto& node : body->nodes) {
        const auto k = node->kind();
        if(k == ASTNodeKind::BreakStmt) {
            return node->as_break_stmt_unsafe()->value.get();
        } else if(k == ASTNodeKind::IfStmt) {
            auto ifStmt = node->as_if_stmt_unsafe();
            value = get_first_broken(&ifStmt->ifBody);
            if(value) return value;
            for(auto& elif : ifStmt->elseIfs) {
                value = get_first_broken(&elif.second);
                if(value) return value;
            }
            if(ifStmt->elseBody.has_value()) {
                value = get_first_broken(&ifStmt->elseBody.value());
                if(value) return value;
            }
        } else if(ASTNode::isLoopASTNode(k)) {
            const auto loopNode = node->as_loop_node_unsafe();
            value = get_first_broken(&loopNode->body);
            if(value) return value;
        } else if(k == ASTNodeKind::SwitchStmt) {
            const auto switchStmt = node->as_switch_stmt_unsafe();
            for(auto& scope : switchStmt->scopes) {
                value = get_first_broken(&scope.second);
                if(value) return value;
            }
            if(switchStmt->defScope.has_value()) {
                value = get_first_broken(&switchStmt->defScope.value());
                if(value) return value;
            }
        }
    }
    return value;
}

Value* LoopBlock::get_first_broken() {
    if(!first_broken) {
        first_broken = ::get_first_broken(&body);
    }
    return first_broken;
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
