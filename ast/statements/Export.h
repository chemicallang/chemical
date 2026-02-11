// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"
#include "std/chem_string_view.h"
#include <vector>

class ExportStmt : public ASTNode {
public:

    /**
     * the identifier list
     */
    std::vector<chem::string_view> ids;

    /**
     * the as identifier
     */
    chem::string_view as_id;

    /**
     * the resolved node
     */
    ASTNode* linked_node = nullptr;

    /**
     * constructor
     */
    ExportStmt(
        chem::string_view as_id,
        ASTNode* parent,
        SourceLocation loc
    ) : ASTNode(ASTNodeKind::ExportStmt, parent, loc), as_id(as_id) {
        // does nothing
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        // does nothing
    }

    void code_gen_external_declare(Codegen &gen) override;

#endif

};
