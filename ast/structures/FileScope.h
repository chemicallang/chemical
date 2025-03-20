// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/structures/Scope.h"

class FileScope : public ASTNode {
public:

    /**
     * the file path
     */
    chem::string_view file_path;

    /**
     * the body contains the nodes inside the body
     */
    Scope body;

    /**
     * constructor
     */
    explicit FileScope(
            ModuleScope* moduleScope
    ) : ASTNode(ASTNodeKind::FileScope, (ASTNode*) moduleScope, 0),
        body(this, 0)
    {

    }

    /**
     * get the parent module scope
     */
    inline ModuleScope* parent() {
        return (ModuleScope*) ASTNode::parent();
    }

    /**
     * move constructor
     */
    FileScope(
        FileScope&& other
    ) noexcept : ASTNode(ASTNodeKind::FileScope, (ASTNode*) other.parent(), 0), file_path(other.file_path), body(std::move(other.body)) {

    }

    /**
     * move assignment
     */
    FileScope& operator =(FileScope&& other) noexcept {
        file_path = other.file_path;
        body = std::move(other.body);
    }

};