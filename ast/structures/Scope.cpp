// Copyright (c) Chemical Language Foundation 2025.

#include "Scope.h"
#include <iostream>
#include "ast/structures/LoopBlock.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/MembersContainer.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/statements/ValueWrapperNode.h"
#include "ast/structures/StructDefinition.h"
#include "ast/values/AccessChain.h"
#include "ast/structures/BaseDefMember.h"
#include "ast/values/FunctionCall.h"
#include "ast/structures/If.h"
#include "ast/statements/SwitchStatement.h"
#include "ast/base/BaseType.h"
#include "ast/statements/Break.h"
#include "compiler/SymbolResolver.h"

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

void Scope::tld_declare(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_top_level(linker, node);
    }
}

void Scope::link_signature(SymbolResolver& linker)  {
    for (const auto node : nodes) {
        node->link_signature(linker);
    }
}

void Scope::declare_and_link(SymbolResolver &linker) {
    for (auto &node: nodes) {
        node->declare_and_link(linker, node);
    }
}

void Scope::link_sequentially(SymbolResolver &linker) {
    if(nodes.empty()) return;
    const auto curr_func = linker.current_func_type;
    const auto moved_ids_begin = curr_func->moved_identifiers.size();
    const auto moved_chains_begin = curr_func->moved_chains.size();
    for(auto& node : nodes) {
        node->declare_top_level(linker, node);
        node->link_signature(linker);
        node->declare_and_link(linker, node);
    }
    if(nodes.back()->kind() == ASTNodeKind::ReturnStmt) {
        curr_func->erase_moved_ids_after(moved_ids_begin);
        curr_func->erase_moved_chains_after(moved_chains_begin);
    }
}

void Scope::stopInterpretOnce() {
    stoppedInterpretOnce = true;
}

void LoopBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    body.link_sequentially(linker);
}

bool LoopBlock::link(SymbolResolver &linker, Value* &value_ptr, BaseType *expected_type) {
    body.link_sequentially(linker);
    return true;
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

BaseType* LoopBlock::create_type(ASTAllocator& allocator) {
    return get_first_broken()->create_type(allocator);
}

BaseType* LoopBlock::known_type() {
    return get_first_broken()->known_type();
}

bool InitBlock::diagnose_missing_members_for_init(ASTDiagnoser& diagnoser) {
    const auto definition = getContainer();
    const auto linked_kind = definition->kind();
    auto& values = initializers;
    if(linked_kind == ASTNodeKind::UnionDecl) {
        if(values.size() != 1) {
            diagnoser.error(this) << "union '" << definition->name_view() << "' must be initialized with a single member value";
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
                diagnoser.error(this) <<
                        "couldn't find value for member '" << miss << "' for initializing struct '" << definition->name_view() << "'";
            }
            return true;
        }
    }
    return false;
}

ExtendableMembersContainerNode* InitBlock::getContainer() {
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
    return parent->as_extendable_members_container_node();
}

void InitBlock::declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) {
    auto mems_container = getContainer();
    if(!mems_container) {
        linker.error("unexpected init block", this);
        return;
    }
    for(auto& in : initializers) {
        in.second.value->link(linker, in.second.value, nullptr);
    }
//    // now taking out initializers
//    for(const auto node : scope.nodes) {
//        const auto val_wrapper = node->as_value_wrapper();
//        if(!val_wrapper) {
//            linker.error("expected members of init block to be initializer call", (ASTNode*) node);
//            continue;
//        }
//        auto chain = val_wrapper->value->as_access_chain();
//        if(!chain) {
//            linker.error("expected members of init block to be initializer call", (ASTNode*) node);
//            continue;
//        }
//        auto& call_ptr = chain->values.back();
//        auto call = call_ptr->as_func_call();
//        if(!call) {
//            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
//            continue;
//        }
////        const auto chain_size = chain->values.size();
////        if(chain_size < 2) {
////            linker.error("expected members of init block to be initializer call", (ASTNode*) chain);
////            continue;
////        }
//        // linking chain till chain_size - 1, last function call is not included
//        // last function call is not linked because it may not be valid and calling struct member
//        if(!chain->link(linker, nullptr, nullptr, 1, false, false)) {
//            continue;
//        }
//        auto call_parent = call->parent_val; // second last value
//        auto linked = call_parent->linked_node();
//        if(!linked) {
//            linker.error("unknown initializer call", (ASTNode*) chain);
//            continue;
//        }
//        auto linked_kind = linked->kind();
//        if(linked_kind == ASTNodeKind::StructMember || linked_kind == ASTNodeKind::UnnamedUnion || linked_kind == ASTNodeKind::UnnamedStruct) {
//            if(call->values.size() != 1) {
//                linker.error("expected a single value to initialize a struct member", (ASTNode*) chain);
//                continue;
//            }
//            auto base_def = linked->as_base_def_member_unsafe();
//            auto& value = call->values.front();
//            value->link(linker, value); // TODO send expected type by getting from base_def
//            initializers[base_def->name] = { false, value };
//            continue;
//        } else if(linked_kind == ASTNodeKind::FunctionDecl) {
//            // linking the last function call, since function call is valid
//            // call_ptr being sent as Value*&, if replaced other than ChainValue, it maybe invalid inside access chain
//            if(!call_ptr->link(linker, (Value*&) call_ptr, nullptr)) {
//                continue;
//            }
//            auto linked_func = linked->as_function();
//            auto func_parent = linked_func->parent();
//            auto called_struc = func_parent ? func_parent->as_struct_def() : nullptr;
//            if(!called_struc) {
//                linker.error("couldn't get struct of constructor in init block", (ASTNode*) chain);
//                continue;
//            }
//            bool found = false;
//            for(auto& inherit : mems_container->inherited) {
//                auto struc = inherit.type->get_direct_linked_struct();
//                if(struc && called_struc == struc) {
//                    found = true;
//                }
//            }
//            if(!found) {
//                linker.error((ASTNode*) chain) << "current struct doesn't inherit struct with name '" << called_struc->name_view() << "'";
//                continue;
//            }
//            initializers[called_struc->name_view()] = { true, chain };
//            continue;
//        } else {
//            linker.error("call to unknown node in init block", (ASTNode*) chain);
//        }
//    }
    diagnose_missing_members_for_init(linker);
}