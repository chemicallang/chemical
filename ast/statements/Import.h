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

    std::vector<chem::string_view> identifiers;
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
        stmt->identifiers = identifiers;
        stmt->as_identifier = as_identifier;
        return stmt;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope);

};