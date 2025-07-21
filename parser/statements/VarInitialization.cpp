// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <cassert>
#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/statements/VarInit.h"
#include "ast/values/PatternMatchExpr.h"
#include "ast/statements/ValueWrapperNode.h"

// if neither a type or a value is given, it would causes errors (in lsp)
VarInitStatement* fix_stmt(VarInitStatement* stmt, TypeBuilder& builder) {
    if(!stmt->type && !stmt->value) {
        stmt->type = { (BaseType*) builder.getVoidType(), ZERO_LOC };
    }
    return stmt;
}

PatternMatchExpr* Parser::parsePatternMatchExprAfterId(
        ASTAllocator& allocator,
        bool is_const,
        bool is_lbrace,
        chem::string_view name_view,
        Token* start_token,
        bool parseElse
) {

    // pattern match expression
    const auto patternMatch = new (allocator.allocate<PatternMatchExpr>()) PatternMatchExpr(
        is_const, is_lbrace, allocate_view(allocator, name_view), loc_single(start_token)
    );

    // lets parse the identifiers
    do {

        const auto id = consumeIdentifierOrKeyword();
        if(!id) break;

        const auto pmId = new (allocator.allocate<PatternMatchIdentifier>()) PatternMatchIdentifier(
            patternMatch, allocate_view(allocator, id->value), parent_node, loc_single(id)
        );

#ifdef LSP_BUILD
        id->linked = (ASTAny*) pmId;
#endif

        patternMatch->param_names.emplace_back(pmId);

    } while(consumeToken(TokenType::CommaSym));

    if(is_lbrace) {
        if(token->type == TokenType::RBrace) {
            token++;
        } else {
            error("expected a right brace after identifier list");
        }
    } else {
        if(token->type == TokenType::RParen) {
            token++;
        } else {
            error("expected a right parenthesis after identifier list");
        }
    }

    if(token->type == TokenType::EqualSym) {
        token++;
    } else {
        error("expected a equal symbol after identifier list");
    }

    const auto expr = parseExpression(allocator, false, false);
    if(expr) {
        patternMatch->expression = expr;
    } else {
        error("expected an expression from which to destructure");
    }

    if(parseElse) {
        if(token->type == TokenType::ElseKw) {
            token++;
        } else {
            error("expected else keyword for else branch");
        }
        switch(token->type) {
            case TokenType::UnreachableKw:
                token++;
                patternMatch->elseExpression.kind = PatternElseExprKind::Unreachable;
                break;
            case TokenType::ReturnKw:{
                token++;
                patternMatch->elseExpression.kind = PatternElseExprKind::Return;
                const auto elseExpr = parseExpression(allocator, true, true);
                if(elseExpr) {
                    patternMatch->elseExpression.value = elseExpr;
                }
                break;
            }
            default:{
                const auto elseExpr = parseExpression(allocator, true, true);
                if(elseExpr) {
                    patternMatch->elseExpression.kind = PatternElseExprKind::DefValue;
                    patternMatch->elseExpression.value = elseExpr;
                } else {
                    error("expected an expression for default value after else in pattern match");
                }
                break;
            }
        }
    }

    return patternMatch;
}

ASTNode* Parser::parseVarInitializationTokens(
        ASTAllocator& allocator,
        AccessSpecifier specifier,
        bool matchExpr,
        bool allowDeclarations,
        bool requiredType
) {

    auto& start_tok = *token;
    auto is_const = start_tok.type == TokenType::ConstKw;
    if(!is_const && start_tok.type != TokenType::VarKw) {
        return nullptr;
    }

    token++;

    const auto is_ref = token->type == TokenType::AmpersandSym;
    if(is_ref) {
        token++;
    }

    auto id = consumeIdentifierOrKeyword();
    if(!id) {
        error("expected an identifier for variable initialization");
        return nullptr;
    }

    if(matchExpr) {
        const auto lType = token->type;
        const auto is_lBrace = lType == TokenType::LBrace;
        if (is_lBrace || lType == TokenType::LParen) {
            // this is a destructuring operation
            token++;
            const auto patternMatchExpr = parsePatternMatchExprAfterId(allocator, is_const, is_lBrace, id->value, &start_tok, true);
            assert(patternMatchExpr != nullptr);

#ifdef LSP_BUILD
            id->linked = patternMatchExpr;
#endif

            const auto wrapperNode = new(allocator.allocate<ValueWrapperNode>()) ValueWrapperNode(
                    patternMatchExpr, parent_node
            );

            return wrapperNode;
        }
    }

    auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(is_const, is_ref, loc_id(allocator, id), nullptr, nullptr, parent_node, loc_single(start_tok), specifier);

#ifdef LSP_BUILD
    id->linked = stmt;
#endif

    auto prev_parent_node = parent_node;
    parent_node = stmt;

    annotate(stmt);

    // :
    if (consumeToken(TokenType::ColonSym)) {

        // type
        stmt->type = parseTypeLoc(allocator);
        if(!stmt->type && requiredType) {
            error("expected type tokens for variable initialization");
            return fix_stmt(stmt, typeBuilder);
        }

    } else if(requiredType) {
        error("expected ':' for type");
        return fix_stmt(stmt, typeBuilder);
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
            return fix_stmt(stmt, typeBuilder);
        }
    }

    // value
    auto expr = parseExpressionOrArrayOrStruct(allocator);
    if(expr) {
        stmt->value = expr;
    } else {
        error("expected an expression / array for variable initialization");
        return fix_stmt(stmt, typeBuilder);
    }

    parent_node = prev_parent_node;

    return stmt;
}