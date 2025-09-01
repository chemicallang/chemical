// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>
#include <unordered_map>
#include "ast/utils/IffyConditional.h"
#include "ast/base/ASTNode.h"

class Diag;

class ASTDiagnoser;

class ImportStatement final : public ASTNode {
public:

    /**
     * a single identifier or a identifier list can be held
     * usually written like this import a from "something" <-- single identifier
     * import a::b::c from "something2", every import statement can import a single decl
     * from a file or from a module, or import a complete file
     */
    std::vector<chem::string_view> identifier;
    chem::string_view filePath; ///< The file path to import.
    chem::string_view as_identifier;
    IffyBase* if_condition = nullptr;
    std::unordered_map<chem::string_view, ASTNode*>* symbols = nullptr;

    /**
     * constructor
     */
    constexpr ImportStatement(
        chem::string_view filePath,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ImportStmt, parent_node, location), filePath(filePath) {

    }

    // we don't use this method at the moment
    // because nothing can be changed inside import statements
    // due to generics
    static IffyBase* copied_iffy(ASTAllocator& allocator, IffyBase* condition) {
        if(condition->is_id) {
            const auto curr = (IffyCondId*) condition;
            const auto expr = allocator.allocate_released<IffyCondId>();
            expr->is_id = true;
            expr->is_negative = curr->is_negative;
            expr->value = curr->value;
        } else {
            const auto curr = (IffyCondExpr*) condition;
            const auto expr = allocator.allocate_released<IffyCondExpr>();
            expr->is_id = false;
            expr->left = copied_iffy(allocator, curr->left);
            expr->right = copied_iffy(allocator, curr->right);
            expr->op = curr->op;
            return expr;
        }
    }

    ImportStatement* copy(ASTAllocator &allocator) override {
        const auto stmt = new (allocator.allocate<ImportStatement>()) ImportStatement(
            filePath,
            parent(),
            encoded_location()
        );
        stmt->identifier = identifier;
        stmt->as_identifier = as_identifier;
        stmt->if_condition = if_condition;
        return stmt;
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {

    }

#endif

    ~ImportStatement() final {
        delete symbols;
    }

};