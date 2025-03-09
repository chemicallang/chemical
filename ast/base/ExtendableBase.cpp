// Copyright (c) Chemical Language Foundation 2025.

#include "ExtendableMembersContainerNode.h"
#include "ast/structures/MembersContainer.h"
#include <sstream>

ASTNode* ExtendableBase::extended_child(const chem::string_view &name) {
    auto func = extension_functions.find(name);
    if(func != extension_functions.end()) {
        auto& pair = func->second;
        return pair.is_generic ? (ASTNode*) pair.decl.gen : (ASTNode*) pair.decl.normal;
    }
    return nullptr;
}

void ExtendableBase::adopt(MembersContainer* definition) {
    for(auto& inherits : definition->inherited) {
        adopt((MembersContainer*) inherits.type->linked_node());
    }
    for(auto& func : definition->functions()) {
        extension_functions[func->name_view()] = { false, { .normal = func } };
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