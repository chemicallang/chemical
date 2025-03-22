// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * the top level package definition in a .mod file is represented with this structure
 */
class PackageDefinition : public ASTNode {
public:

    chem::string_view scope_name;

    chem::string_view module_name;

    /**
     * constructor
     */
    PackageDefinition(
            chem::string_view scope_name,
            chem::string_view module_name,
            SourceLocation loc
    ) : ASTNode(ASTNodeKind::PackageDef, nullptr, loc),
        scope_name(scope_name), module_name(module_name)
    {

    }

};