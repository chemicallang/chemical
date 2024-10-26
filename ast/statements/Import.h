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

    std::vector<std::string> identifiers;
    std::string filePath; ///< The file path to import.
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * @brief Construct a new ImportStatement object.
     *
     * @param filePath The file path to import.
     */
    ImportStatement(
        std::string filePath,
        std::vector<std::string> identifiers,
        ASTNode* parent_node,
        SourceLocation location
    );

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