// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

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
     * constructor
     */
    ModuleScope(
        chem::string_view scope_name,
        chem::string_view module_name
    ) : ASTNode(ASTNodeKind::ModuleScope, nullptr, 0),
        scope_name(scope_name), module_name(module_name)
    {

    }

};