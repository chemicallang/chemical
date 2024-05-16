// Copyright (c) Qinetik 2024.

#pragma once

#include "cst/base/CSTVisitor.h"
#include "BaseSymbolResolver.h"
#include <vector>
#include <unordered_map>
#include <string>

class CSTSymbolResolver : public BaseSymbolResolver<CSTToken>, public CSTVisitor {
public:

    /**
     * when traversing nodes, a node can declare itself on the map
     * this is vector of scopes, the last scope is current scope
     */
    std::vector<std::unordered_map<std::string, CSTToken*>> current = {{}};

    /**
     * declares a node with string : name
     */
    void declare(const std::string &name, CSTToken *node);

};