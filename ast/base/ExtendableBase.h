// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

class ASTNode;

class FunctionDeclaration;

class MembersContainer;

class ExtendableBase {
public:

    /**
     * map suppose to contain references to extension functions
     */
    std::unordered_map<std::string, FunctionDeclaration*> extension_functions;

    /**
     * get the child member by name
     * this will also look for extension function by name, if there's no direct member
     */
    FunctionDeclaration *extended_child(const std::string &name);

    /**
     * all the methods of this interface will be extended to this
     * struct / member, so user can call it using dot notation obj.method
     */
    void adopt(MembersContainer* definition);

};