// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 16/02/2024.
//

#include <cassert>
#include <cstring>
#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/statements/VarInit.h"
#include "ast/statements/PatternMatchExprNode.h"
#include "ast/base/ASTNodeKind.h"

// if neither a type or a value is given, it would causes errors (in lsp)
VarInitStatement* fix_stmt(VarInitStatement* stmt, TypeBuilder& builder) {
    if(!stmt->type && !stmt->value) {
        stmt->type = { (BaseType*) builder.getVoidType(), ZERO_LOC };
    }
    return stmt;
}

void Parser::parsePatternMatchExprAfterId(
        ASTAllocator& allocator,
        PatternMatchExpr* patternMatch,
        bool is_lbrace,
        bool parseElse
) {

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
        patternMatch->expression = getErroredValue(allocator);
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
            case TokenType::BreakKw: {
                token++;
                patternMatch->elseExpression.kind = PatternElseExprKind::Break;
                const auto elseExpr = parseExpression(allocator, true, true);
                if (elseExpr) {
                    patternMatch->elseExpression.value = elseExpr;
                }
                break;
            }
            case TokenType::ContinueKw: {
                token++;
                patternMatch->elseExpression.kind = PatternElseExprKind::Continue;
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

}

ASTNode* Parser::parseVarInitializationTokens(
        ASTAllocator& allocator,
        AccessSpecifier specifier,
        bool topLevel,
        bool matchExpr,
        bool allowDeclarations,
        bool comptime,
        bool is_unsafe
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

            const auto patternMatchExpr = new (allocator.allocate<PatternMatchExprNode>()) PatternMatchExprNode(
                    is_const, is_lBrace, allocate_view(allocator, id->value), loc_single(start_tok), parent_node
            );

             parsePatternMatchExprAfterId(allocator, &patternMatchExpr->value, is_lBrace, true);

#ifdef LSP_BUILD
            id->linked = patternMatchExpr;
#endif

            return patternMatchExpr;
        }
    }

    auto stmt = new (allocator.allocate<VarInitStatement>()) VarInitStatement(is_const, is_ref, loc_id(allocator, id), nullptr, nullptr, parent_node, loc_single(id), specifier);

    if(comptime) {
        stmt->set_comptime(true);
    }

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
        if (stmt->type) {
            stmt->attrs.user_gave_type = true;
        }

    }

    // equal sign
    if (!consumeToken(TokenType::EqualSym)) {
        // An uninitialized declaration (no initializer) is now allowed without
        // any keyword — the definite-assignment analysis (run in the type-verify
        // pass) will error if such a variable is accessed before it is assigned.
        //
        // The old `unsafe var x : Type` syntax is accepted (for backwards
        // compatibility) but deprecated: `unsafe` at the declaration site no
        // longer toggles any flag or affects safe/unsafe checking — it is just
        // parsed through. An uninitialized variable does not need `unsafe` here.
        // To take a pointer/reference to an uninitialized variable, wrap the
        // expression in `unsafe(...)` instead, e.g. `unsafe(&raw mut x)`.
        if(is_unsafe) {
            // Emit the deprecation warning for user / test code, but keep it
            // quiet inside the standard library: `lang/libs` still has many
            // `unsafe var` sites that are migrated in bulk, and warning on every
            // one would flood every compilation (the std library is auto-imported
            // into essentially every module).
            auto fp = get_file_path();
            bool is_library = false;
            {
                const char* needle = "lang/libs";
                const size_t nlen = 9;
                if(fp.size() >= nlen) {
                    for(size_t i = 0; i + nlen <= fp.size(); i++) {
                        if(std::memcmp(fp.data() + i, needle, nlen) == 0) {
                            is_library = true;
                            break;
                        }
                    }
                }
            }
            if(!is_library) {
                warning("'unsafe var' / 'unsafe const' is deprecated; write 'var x : Type' "
                        "(uninitialized variables no longer need the 'unsafe' keyword — take "
                        "its address with unsafe(...), e.g. unsafe(&raw mut x))");
            }
            stmt->attrs.is_unsafe = true;
        }
        if(
            // for loop sends false
            allowDeclarations == false ||
            // local const variable must be defined then and there
            (topLevel == false && is_const)
        ) {

            error("expected an = sign for the initialization of the variable");
            return stmt;
        }
        if(stmt->type) {
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