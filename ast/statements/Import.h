// Copyright (c) Qinetik 2024.

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
    ASTNode* parent_node;

    /**
     * constructor
     */
    ImportStatement(
        chem::string_view filePath,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ImportStmt, location), filePath(filePath), parent_node(parent_node) {

    }


    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

    void declare_top_level(SymbolResolver &linker, ASTNode*& node_ptr) final;

    void interpret(InterpretScope &scope);

};