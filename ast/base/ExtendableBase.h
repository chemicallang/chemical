// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class ASTNode;

class ExtensionFunction;

class ExtendableBase {
public:

    /**
     * map suppose to contain references to extension functions
     */
    std::unordered_map<std::string, ExtensionFunction*> extension_functions;

    /**
     * get the child member by name
     * this will also look for extension function by name, if there's no direct member
     */
    ASTNode *extended_child(const std::string &name);

};