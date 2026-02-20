// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/structures/Scope.h"
#include "compiler/processor/ASTFileMetaData.h"

class FileScope : public ASTNode {
public:

    /**
     * the pointer back to file result
     */
    ASTFileMetaData& meta;

    /**
     * the body contains the nodes inside the body
     */
    Scope body;

    /**
     * constructor
     */
    explicit constexpr FileScope(
            ASTFileMetaData& meta,
            ModuleScope* moduleScope
    ) : ASTNode(ASTNodeKind::FileScope, (ASTNode*) moduleScope, 0),
        meta(meta), body(this, 0) {

    }

    /**
     * get the file id for this file scope
     */
    inline unsigned int getFileId() {
        return meta.file_id;
    }

    /**
     * get the absolute path to this file
     */
    inline const std::string& getAbsPath() {
        return meta.abs_path;
    }

    /**
     * get the parent module scope
     */
    inline ModuleScope* parent() {
        return (ModuleScope*) ASTNode::parent();
    }

    /**
     * set new parent
     */
    inline void set_parent(ModuleScope* parent) {
        return ASTNode::set_parent((ASTNode*) parent);
    }

};