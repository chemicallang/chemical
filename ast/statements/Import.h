// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/ASTNode.h"
#include "utils/fwd/functional.h"

class Diag;

class ASTDiagnoser;

class ImportStatement : public ASTNode {
public:

    std::vector<chem::string_view> identifiers;
    chem::string_view filePath; ///< The file path to import.
    chem::string_view as_identifier;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * constructor
     */
    ImportStatement(
        chem::string_view filePath,
        ASTNode* parent_node,
        SourceLocation location
    ) : filePath(filePath), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ImportStmt;
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

    void declare_top_level(SymbolResolver &linker) final;

    void interpret(InterpretScope &scope);

};