// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Comment : public ASTNode {
public:

    ASTNode* parent_node;
    chem::string_view comment;
    bool multiline;
    SourceLocation location;

    Comment(
            chem::string_view comment,
            bool multiline,
            ASTNode* parent,
            SourceLocation location
    ) : comment(comment), multiline(multiline), parent_node(parent), location(location) {}

    ASTNodeKind kind() final {
        return ASTNodeKind::CommentStmt;
    }

    SourceLocation encoded_location() final {
        return location;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void interpret(InterpretScope &scope) final {
        // do nothing, since it's a comment
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final {
        // doesn't generate anything
    }
#endif

};