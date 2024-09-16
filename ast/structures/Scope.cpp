// Copyright (c) Qinetik 2024.

#include "Scope.h"
#include <iostream>
#include "ast/base/GlobalInterpretScope.h"
#include "ast/structures/LoopBlock.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/values/FunctionCall.h"
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

InitBlock::InitBlock(Scope scope, ASTNode* parent_node, CSTToken* token) : scope(std::move(scope)), parent_node(parent_node), token(token) {

}

void InitBlock::declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) {
    auto func = parent_node->as_function();
    if(!func) {
        linker.error("expected init block to be in a function", (ASTNode*) this);
        return;
    }
    if(!func->has_annotation(AnnotationKind::Constructor)) {
        linker.error("init block must appear in a function that's marked constructor", (ASTNode*) this);
        return;
    }
    func_decl = func;
    auto parent = func->parent_node;
    if(!parent) {
        linker.error("init block's function must be inside a struct", (ASTNode*) this);
        return;
    }
    auto mems_container = parent->as_members_container();
    if(!mems_container) {
        linker.error("init block's function must be inside a struct", (ASTNode*) this);
        return;
    }
    container = mems_container;
    // now taking out initializers
    for(auto& node : scope.nodes) {
        auto chain = node->as_access_chain();
        if(!chain) {
            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
            continue;
        }
        auto& call_ptr = chain->values.back();
        auto call = call_ptr->as_func_call();
        if(!call) {
            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
            continue;
        }
        const auto chain_size = chain->values.size();
        if(chain_size < 2) {
            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
            continue;
        }
        // linking chain till chain_size - 1, last function call is not included
        // last function call is not linked because it may not be valid and calling struct member
        if(!chain->link(linker, nullptr, nullptr, 1)) {
            continue;
        }
        auto call_parent = chain->values[chain_size - 2].get(); // second last value
        auto linked = call_parent->linked_node();
        if(!linked) {
            linker.error("unknown initializer call", (ASTNode*) chain);
            continue;
        }
        auto linked_kind = linked->kind();
        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
            if(call->values.size() != 1) {
                linker.error("expected a single value to initialize a struct member", (ASTNode*) chain);
                continue;
            }
            auto base_def = linked->as_base_def_member_unsafe();
            auto& value = call->values.front();
            value->link(linker, value); // TODO send expected type by getting from base_def
            initializers[base_def->name] = { false, value.get() };
            continue;
        } else if(linked_kind == ASTNodeKind::FunctionDecl) {
            // linking the last function call, since function call is valid
            if(!call_ptr->link(linker, call_parent, chain->values, chain_size - 1, nullptr)) {
                continue;
            }
            auto linked_func = linked->as_function();
            auto func_parent = linked_func->parent_node;
            auto called_struc = func_parent ? func_parent->as_struct_def() : nullptr;
            if(!called_struc) {
                linker.error("couldn't get struct of constructor in init block", (ASTNode*) chain);
                continue;
            }
            bool found = false;
            for(auto& inherit : mems_container->inherited) {
                auto struc = inherit->type->get_direct_linked_struct();
                if(struc && called_struc == struc) {
                    found = true;
                }
            }
            if(!found) {
                linker.error("current struct doesn't inherit struct with name '" + called_struc->name + "'", (ASTNode*) chain);
                continue;
            }
            initializers[called_struc->name] = { true, chain };
            continue;
        } else {
            linker.error("call to unknown node in init block", (ASTNode*) chain);
        }
    }
}