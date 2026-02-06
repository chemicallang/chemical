// Copyright (c) Chemical Language Foundation 2025.

#include "Scope.h"
#include <iostream>
#include "ast/structures/LoopBlock.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/ImplDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/GenericUnionDecl.h"
#include "ast/structures/GenericInterfaceDecl.h"
#include "ast/structures/GenericImplDecl.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/Namespace.h"
#include "ast/values/AccessChain.h"
#include "ast/values/FunctionCall.h"
#include "ast/structures/If.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/base/BaseType.h"
#include "ast/statements/Break.h"
#include "compiler/ASTDiagnoser.h"
#include "std/except.h"

// deduplicate the nodes, so nodes with same id appearing later will override the nodes appearing before them
void top_level_dedupe(std::vector<ASTNode*>& nodes) {

    std::vector<ASTNode*> reverse_nodes;
    std::unordered_map<chem::string_view, unsigned int> dedupe;

    reverse_nodes.reserve(nodes.size());
    dedupe.reserve(nodes.size());

    const auto nodes_size = nodes.size();
    int i = ((int) nodes_size) - 1;
    while(i >= 0) {
        auto node = nodes[i];
        const auto node_id = node->get_located_id();
        if(node_id) {
            if(dedupe.find(node_id->identifier) == dedupe.end()) {
                reverse_nodes.emplace_back(node);
                dedupe[node_id->identifier] = i;
            }
        }
        i--;
    }

    nodes.clear();

    // put nodes from reverse nodes into nodes
    i = ((int) reverse_nodes.size()) - 1;
    while(i >= 0) {
        nodes.emplace_back(reverse_nodes[i]);
        i--;
    }

}

void make_exportable(std::vector<ASTNode*>& nodes) {
    std::vector<ASTNode*> public_nodes;
    public_nodes.reserve(nodes.size());
    for(const auto node : nodes) {
        switch(node->kind()) {
            case ASTNodeKind::IfStmt:{
                const auto stmt = node->as_if_stmt_unsafe();
                if(stmt->computed_scope.has_value()) {
                    const auto scope = stmt->computed_scope.value();
                    if(scope) {
                        make_exportable(scope->nodes);
                        if(!scope->nodes.empty()) {
                            public_nodes.emplace_back(node);
                        }
                    }
                } else {
#ifdef DEBUG
                    CHEM_THROW_RUNTIME("unevaluated top level if statement");
#endif
                }
                break;
            }
            case ASTNodeKind::NamespaceDecl:{
                const auto ns = node->as_namespace_unsafe();
                if(is_linkage_public(ns->specifier())) {
                    make_exportable(ns->nodes);
                    public_nodes.emplace_back(node);
                }
                break;
            }
            default: {
                if(is_linkage_public(node->specifier())) {
                    public_nodes.emplace_back(node);
                }
                break;
            }
        }
    }
    nodes = std::move(public_nodes);
}

void Scope::stopInterpretOnce() {
    stoppedInterpretOnce = true;
}

Value* get_first_broken(Scope* body) {
    Value* value = nullptr;
    for(const auto node : body->nodes) {
        const auto k = node->kind();
        if(k == ASTNodeKind::BreakStmt) {
            return node->as_break_stmt_unsafe()->value;
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
                value = get_first_broken(&scope);
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

BaseType* LoopBlock::known_type() {
    return get_first_broken()->getType();
}