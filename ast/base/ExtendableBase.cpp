// Copyright (c) Chemical Language Foundation 2025.

#include "ExtendableMembersContainerNode.h"
#include "ast/structures/MembersContainer.h"
#include <sstream>

ASTNode* ExtendableBase::extended_child(const chem::string_view &name) {
    auto func = extension_functions.find(name);
    return func != extension_functions.end() ? func->second : nullptr;
}

void ExtendableBase::adopt(MembersContainer* definition) {
    for(auto& inherits : definition->inherited) {
        adopt((MembersContainer*) inherits.type->linked_node());
    }
    for(const auto node : definition->evaluated_nodes()) {
        switch(node->kind()) {
            case ASTNodeKind::FunctionDecl:
                extension_functions[node->as_function_unsafe()->name_view()] = node;
                break;
            case ASTNodeKind::GenericFuncDecl:
                extension_functions[node->as_gen_func_decl_unsafe()->master_impl->name_view()] = node;
                break;
            default:
                break;
        }
    }
}

void ExtendableMembersContainerNode::runtime_name_no_parent(std::ostream &stream) {
    if(generic_instantiation != -1) {
        stream << name_view();
        stream << "__cgs__";
        stream << generic_instantiation;
    } else {
        stream << name_view();
    }
}

std::string ExtendableMembersContainerNode::runtime_name() {
    std::stringstream s;
    runtime_name(s);
    return s.str();
}

std::string ExtendableMembersContainerNode::runtime_name_no_parent_str() {
    std::stringstream s;
    runtime_name_no_parent(s);
    return s.str();
}

void ExtendableMembersContainerNode::runtime_name(std::ostream &stream) {
    const auto p = parent();
    if(p) p->runtime_name(stream);
    ExtendableMembersContainerNode::runtime_name_no_parent(stream);
}