// Copyright (c) Qinetik 2024.

#include "ExtendableMembersContainerNode.h"
#include "ast/structures/MembersContainer.h"

FunctionDeclaration *ExtendableBase::extended_child(const std::string &name) {
    auto func = extension_functions.find(name);
    if(func != extension_functions.end()) {
        return func->second;
    }
    return nullptr;
}

void ExtendableBase::adopt(MembersContainer* definition) {
    for(auto& inherits : definition->inherited) {
        adopt((MembersContainer*) inherits->type->linked_node());
    }
    for(auto& func : definition->functions()) {
        extension_functions[func->name] = func.get();
    }
}

void ExtendableMembersContainerNode::runtime_name_no_parent(std::ostream &stream) {
    if(generic_params.empty()) {
        stream << name;
    } else {
        stream << name;
        stream << "__cgs__";
        stream << active_iteration;
    }
}