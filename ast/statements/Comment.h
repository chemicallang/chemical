// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 09/03/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Comment : public ASTNode {
public:

    chem::string_view comment;
    bool multiline;

    constexpr Comment(
            chem::string_view comment,
            bool multiline,
            ASTNode* parent,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::CommentStmt, parent, location), comment(comment), multiline(multiline) {}


    Comment* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<Comment>()) Comment(
            comment,
            parent(),
            parent(),
            encoded_location()
        );
    }

    void interpret(InterpretScope &scope) final {
        // do nothing, since it's a comment
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) final {
        // doesn't generate anything
    }
#endif

};