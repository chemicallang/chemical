// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

struct LabModule;

class ModuleScope : public ASTNode {
public:

    /**
     * the scope name refers to usually the organization name in the github / username
     */
    chem::string_view scope_name;

    /**
     * the module name refers to the module name / repo name on github
     */
    chem::string_view module_name;

    /**
     * the pointer to module, inside which this scope exists
     * note: scope can be different than the module
     */
    LabModule* container;

    /**
     * this is calculated when module has been completely symbol resolved
     * it contains all the top level accessible symbols in the module
     */
    ChildrenMapNode* children = nullptr;

    /**
     * constructor
     */
    ModuleScope(
        chem::string_view scope_name,
        chem::string_view module_name,
        LabModule* container
    ) : ASTNode(ASTNodeKind::ModuleScope, nullptr, 0),
        scope_name(scope_name), module_name(module_name), container(container)
    {

    }

};