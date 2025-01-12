// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include "parser/Parser.h"
#include "ast/statements/VarInit.h"

VarInitStatement* Parser::parseVarInitializationTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool allowDeclarations, bool requiredType) {

    auto& start_tok = *token;
    auto is_const = start_tok.type == TokenType::ConstKw;
    if(!is_const && start_tok.type != TokenType::VarKw) {
        return nullptr;
    }

    token++;

    auto id = consumeIdentifierOrKeyword();
    if(!id) {
        error("expected an identifier for variable initialization");
        return nullptr;
    }

    auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(is_const, loc_id(allocator, id), nullptr, nullptr, parent_node, loc_single(start_tok), specifier);

    auto prev_parent_node = parent_node;
    parent_node = stmt;

    annotate(stmt);

    // :
    if (consumeToken(TokenType::ColonSym)) {

        // type
        stmt->type = parseType(allocator);
        if(!stmt->type && requiredType) {
            error("expected type tokens for variable initialization");
            return stmt;
        }

    } else if(requiredType) {
        error("expected ':' for type");
        return stmt;
    }

    // equal sign
    if (!consumeToken(TokenType::EqualSym)) {
        if(!allowDeclarations) {
            error("expected an = sign for the initialization of the variable");
            return stmt;
        } else if(stmt->type) {
            parent_node = prev_parent_node;
            return stmt;
        } else {
            error("a type or value is required to initialize a variable");
            return stmt;
        }
    }

    // value
    auto expr = parseExpression(allocator, true);
    if(expr) {
        stmt->value = expr;
    } else {
        auto init = parseArrayInit(allocator);
        if(init) {
            stmt->value = init;
        } else {
            error("expected an expression / array for variable initialization");
            return stmt;
        }
    }

    parent_node = prev_parent_node;

    return stmt;
}