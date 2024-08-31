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
    CSTToken* token;

    /**
     * @brief Construct a new ImportStatement object.
     *
     * @param filePath The file path to import.
     */
    ImportStatement(
        std::string filePath,
        std::vector<std::string> identifiers,
        ASTNode* parent_node,
        CSTToken* token
    );

    CSTToken* cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ImportStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

    void declare_top_level(SymbolResolver &linker, std::unique_ptr<ASTNode>& node_ptr) override;

    void interpret(InterpretScope &scope);

};