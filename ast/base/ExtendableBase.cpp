// Copyright (c) Qinetik 2024.

#include "ExtendableBase.h"
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