// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"

class Diag;

class ASTDiagnoser;

class ImportStatement : public ASTNode {
public:

    /**
     * a single identifier or a identifier list can be held
     * usually written like this import a from "something" <-- single identifier
     * import a::b::c from "something2", every import statement can import a single identifier
     * from a file from a module or the file itself
     */
    std::vector<chem::string_view> identifier;
    chem::string_view filePath; ///< The file path to import.
    chem::string_view as_identifier;


    /**
     * constructor
     */
    constexpr ImportStatement(
        chem::string_view filePath,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ImportStmt, parent_node, location), filePath(filePath) {

    }

    ImportStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement(
            filePath,
            parent(),
            encoded_location()
        );
        stmt->identifier = identifier;
        stmt->as_identifier = as_identifier;
        return stmt;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

};