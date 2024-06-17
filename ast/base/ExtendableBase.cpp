// Copyright (c) Qinetik 2024.

#include "ExtendableBase.h"

ASTNode *ExtendableBase::extended_child(const std::string &name) {
    auto func = extension_functions.find(name);
    if(func != extension_functions.end()) {
        return (ASTNode*) func->second;
    }
    return nullptr;
}