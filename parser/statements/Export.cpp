// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/statements/Export.h"

ASTNode* Parser::parseExportStatement(ASTAllocator& passed_allocator) {
    auto& allocator = global_allocator;
    const auto loc = loc_single(token);
    token++; // move past 'export'

    const auto stmt = new (allocator.allocate<ExportStmt>()) ExportStmt("", nullptr, loc);;

    auto& ids = stmt->ids;
    if (token->type == TokenType::Identifier) {
        ids.push_back(allocate_view(allocator, token->value));
        token++;
        while (token->type == TokenType::DoubleColonSym) {
            token++; // move past '::'
            if (token->type == TokenType::Identifier) {
                ids.push_back(allocate_view(allocator, token->value));
                token++;
            } else {
                error("expected identifier after '::' in export statement");
                return nullptr;
            }
        }
    } else {
        error("expected identifier in export statement");
        return nullptr;
    }

    if (token->type == TokenType::AsKw) {
        token++; // move past 'as'
        if (token->type == TokenType::Identifier) {
            stmt->as_id = allocate_view(allocator, token->value);
            token++;
        } else {
            error("expected identifier after 'as' in export statement");
            return nullptr;
        }
    }

    return stmt;

}
