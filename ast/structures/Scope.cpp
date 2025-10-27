// Copyright (c) Chemical Language Foundation 2025.

#include "Scope.h"
#include <iostream>
#include "ast/structures/LoopBlock.h"
#include "ast/structures/InitBlock.h"
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
                if(ns->specifier() == AccessSpecifier::Public) {
                    make_exportable(ns->nodes);
                    public_nodes.emplace_back(node);
                }
                break;
            }
            default: {
                const auto is_public = node->specifier() == AccessSpecifier::Public;
                if(is_public) {
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

bool InitBlock::diagnose_missing_members_for_init(ASTDiagnoser& diagnoser) {
    const auto definition = getContainer();
    const auto linked_kind = definition->kind();
    auto& values = initializers;
    if(linked_kind == ASTNodeKind::UnionDecl) {
        if(values.size() != 1) {
            diagnoser.error(this) << "union must be initialized with a single member value";
            return false;
        } else {
            return true;
        }
    }
    if(values.size() < definition->init_values_req_size()) {
        std::vector<chem::string_view> missing;
        for(auto& mem : definition->inherited) {
            auto& type = *mem.type;
            if(type.get_direct_linked_struct()) {
                const auto& ref_type_name = mem.ref_type_name();
                auto val = values.find(ref_type_name);
                if (val == values.end()) {
                    missing.emplace_back(ref_type_name);
                }
            }
        }
        for(const auto mem : definition->variables()) {
            if(mem->default_value() == nullptr) {
                auto val = values.find(mem->name);
                if (val == values.end()) {
                    missing.emplace_back(mem->name);
                }
            }
        }
        if(!missing.empty()) {
            for (auto& miss: missing) {
                diagnoser.error(this) << "couldn't find value for member '" << miss << "' for initializing struct";
            }
            return true;
        }
    }
    return false;
}

MembersContainer* InitBlock::getContainer() {
    auto func = parent()->as_function();
    if(!func) {
        return nullptr;
    }
    if(!func->is_constructor_fn()) {
        return nullptr;
    }
    auto parent = func->parent();
    if(!parent) {
        return nullptr;
    }
    switch(parent->kind()) {
        case ASTNodeKind::StructDecl:
        case ASTNodeKind::InterfaceDecl:
        case ASTNodeKind::UnionDecl:
        case ASTNodeKind::VariantDecl:
        case ASTNodeKind::ImplDecl:
            return parent->as_extendable_members_container_unsafe();
        case ASTNodeKind::GenericUnionDecl:
            return parent->as_gen_union_decl_unsafe()->master_impl;
        case ASTNodeKind::GenericStructDecl:
            return parent->as_gen_struct_def_unsafe()->master_impl;
        case ASTNodeKind::GenericInterfaceDecl:
            return parent->as_gen_interface_decl_unsafe()->master_impl;
        case ASTNodeKind::GenericVariantDecl:
            return parent->as_gen_variant_decl_unsafe()->master_impl;
        case ASTNodeKind::GenericImplDecl:
            return parent->as_gen_impl_decl_unsafe()->master_impl;
        default:
            return nullptr;
    }
}