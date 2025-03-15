// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <memory>
#include <string>
#include "std/chem_string_view.h"
#include <unordered_map>

class ASTNode;

class FunctionDeclaration;

class MembersContainer;

class GenericFuncDecl;

class ExtendableBase {
public:

    /**
     * map suppose to contain references to extension functions
     */
    std::unordered_map<chem::string_view, ASTNode*> extension_functions;

    /**
     * add an extension function
     */
    void add_extension_func(const chem::string_view& name, FunctionDeclaration* decl) {
        extension_functions[name] = (ASTNode*) decl;
    }

    /**
     * add extension function
     */
    void add_extension_func(const chem::string_view& name, GenericFuncDecl* decl) {
        extension_functions[name] = (ASTNode*) decl;
    }

    /**
     * get the child member by name
     * this will also look for extension function by name, if there's no direct member
     */
    ASTNode *extended_child(const chem::string_view &name);

    /**
     * all the methods of this interface will be extended to this
     * struct / member, so user can call it using dot notation obj.method
     */
    void adopt(MembersContainer* definition);

};