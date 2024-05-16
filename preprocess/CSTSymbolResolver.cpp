// Copyright (c) Qinetik 2024.

#include "CSTSymbolResolver.h"

void CSTSymbolResolver::declare(const std::string &name, CSTToken *node) {
    auto& last = current.back();
    auto found = last.find(name);
    if(found == last.end()) {
        last[name] = node;
    } else {
//        std::string err("duplicate symbol being declared " + name + " symbol already exists\n");
//        err.append("previous : " + found->second->representation() + "\n");
//        err.append("new : " + node->representation() + "\n");
//        error(err);
    }
}