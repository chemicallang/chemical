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

bool Parser::parseArrayPatternElements(ASTAllocator& allocator, PatternMatchExpr* into) {
    into->is_array_pattern = true;
    int32_t pattern_index = 0;
    while(true) {
        consumeNewLines();
        if(token->type == TokenType::RBracket) break;
        if(token->type == TokenType::DotDotSym) {
            if(into->array_rest_index >= 0) {
                error("an array pattern can contain only one '..'");
            } else {
                into->array_rest_index = pattern_index;
            }
            token++;
        } else {
            const auto element_id = consumeIdentifierOrKeyword();
            if(!element_id) {
                error("expected an identifier or '..' in array pattern");
                return false;
            }
            const auto pmId = new (allocator.allocate<PatternMatchIdentifier>())
                PatternMatchIdentifier(into, allocate_view(allocator, element_id->value),
                                       parent_node, loc_single(element_id));
            pmId->is_wildcard = element_id->value == "_";
            into->param_names.emplace_back(pmId);
        }
        pattern_index++;
        if(consumeToken(TokenType::CommaSym)) continue;
        break;
    }
    if(!consumeToken(TokenType::RBracket)) {
        error("expected ']' after array pattern");
        return false;
    }
    return true;
}

bool Parser::parsePatternElse(ASTAllocator& allocator, PatternMatchExpr* patternMatch) {
    if(token->type == TokenType::ElseKw) {
        token++;
    } else {
        error("expected else keyword for else branch");
        return false;
    }
    // `else unreachable` is a trap, so it is an unsafe boundary. The explicit
    // form is `else unsafe { unreachable }` (a bare `else unsafe unreachable`
    // is accepted too). This keeps the binding in the enclosing scope while
    // making the trap auditable.
    if(token->type == TokenType::UnsafeKw) {
        token++;
        patternMatch->elseExpression.is_unsafe = true;
        if(token->type == TokenType::LBrace) {
            token++;
            consumeNewLines();
            if(token->type != TokenType::UnreachableKw) {
                error("expected 'unreachable' inside 'else unsafe'");
                return false;
            }
            token++;
            consumeToken(TokenType::SemiColonSym);
            consumeNewLines();
            if(token->type != TokenType::RBrace) {
                error("expected '}' after 'else unsafe { unreachable }'");
                return false;
            }
            token++;
        } else if(token->type == TokenType::UnreachableKw) {
            token++;
        } else {
            error("expected 'unreachable' after 'else unsafe'");
            return false;
        }
        patternMatch->elseExpression.kind = PatternElseExprKind::Unreachable;
        return true;
    }
    switch(token->type) {
        case TokenType::UnreachableKw:
            token++;
            patternMatch->elseExpression.kind = PatternElseExprKind::Unreachable;
            break;
        case TokenType::ReturnKw: {
            token++;
            patternMatch->elseExpression.kind = PatternElseExprKind::Return;
            const auto elseExpr = parseExpression(allocator, true, true);
            if(elseExpr) patternMatch->elseExpression.value = elseExpr;
            break;
        }
        case TokenType::BreakKw: {
            token++;
            patternMatch->elseExpression.kind = PatternElseExprKind::Break;
            const auto elseExpr = parseExpression(allocator, true, true);
            if(elseExpr) patternMatch->elseExpression.value = elseExpr;
            break;
        }
        case TokenType::ContinueKw: {
            token++;
            patternMatch->elseExpression.kind = PatternElseExprKind::Continue;
            break;
        }
        default: {
            const auto elseExpr = parseExpression(allocator, true, true);
            if(elseExpr) {
                patternMatch->elseExpression.kind = PatternElseExprKind::DefValue;
                patternMatch->elseExpression.value = elseExpr;
            } else {
                error("expected an expression for default value after else in pattern match");
                return false;
            }
            break;
        }
    }
    return true;
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
            parsePatternElse(allocator, patternMatch);
        } else {
            error("expected else keyword for else branch");
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
        bool static_decl,
        bool static_mut
) {

    auto* start_tok = token;
    auto is_const = static_decl ? !static_mut : start_tok->type == TokenType::ConstKw;
    if(!static_decl && !is_const && start_tok->type != TokenType::VarKw) {
        return nullptr;
    }

    if(!static_decl) {
        token++;
    }

    const auto is_ref = token->type == TokenType::AmpersandSym;
    if(is_ref) {
        token++;
    }

    if(matchExpr && token->type == TokenType::LBracket) {
        // Fixed-array destructuring: `var [a, b] = arr`, optionally with one
        // `..` at any position (`[first, .., last]`).
        const auto patternMatchExpr = new (allocator.allocate<PatternMatchExprNode>())
            PatternMatchExprNode(is_const, false, chem::string_view(""),
                                 loc_single(start_tok), parent_node);
        token++; // '['
        parseArrayPatternElements(allocator, &patternMatchExpr->value);
        if(!consumeToken(TokenType::EqualSym)) {
            error("expected '=' after array pattern");
            patternMatchExpr->value.expression = getErroredValue(allocator);
        } else {
            const auto expr = parseExpressionOrArrayOrStruct(allocator);
            if(expr) {
                patternMatchExpr->value.expression = expr;
            } else {
                error("expected an expression after '=' in array pattern");
                patternMatchExpr->value.expression = getErroredValue(allocator);
            }
        }
        if(token->type == TokenType::ElseKw) {
            parsePatternElse(allocator, &patternMatchExpr->value);
        }
        return patternMatchExpr;
    }

    if(matchExpr && token->type == TokenType::LParen) {
        // Tuple destructuring: `var (a, b) = value`. This is irrefutable, so
        // unlike a variant pattern it has no `else` branch.
        const auto patternMatchExpr = new (allocator.allocate<PatternMatchExprNode>())
            PatternMatchExprNode(is_const, false, chem::string_view(""),
                                 loc_single(start_tok), parent_node);
        patternMatchExpr->value.is_tuple = true;
        token++; // '('
        uint32_t tuple_index = 0;
        while(true) {
            consumeNewLines();
            if(token->type == TokenType::RParen) break;
            const auto tuple_id = consumeIdentifierOrKeyword();
            if(!tuple_id) {
                error("expected an identifier in tuple pattern");
                break;
            }
            const auto pmId = new (allocator.allocate<PatternMatchIdentifier>())
                PatternMatchIdentifier(&patternMatchExpr->value,
                                       allocate_view(allocator, tuple_id->value),
                                       parent_node, loc_single(tuple_id));
            pmId->tuple_index = tuple_index++;
            pmId->is_wildcard = tuple_id->value == "_";
            patternMatchExpr->value.param_names.emplace_back(pmId);
            if(consumeToken(TokenType::CommaSym)) continue;
            break;
        }
        if(!consumeToken(TokenType::RParen)) {
            error("expected ')' after tuple pattern");
        }
        if(!consumeToken(TokenType::EqualSym)) {
            error("expected '=' after tuple pattern");
            patternMatchExpr->value.expression = getErroredValue(allocator);
        } else {
            const auto expr = parseExpressionOrArrayOrStruct(allocator);
            if(expr) {
                patternMatchExpr->value.expression = expr;
            } else {
                error("expected an expression after '=' in tuple pattern");
                patternMatchExpr->value.expression = getErroredValue(allocator);
            }
        }
        if(token->type == TokenType::ElseKw) {
            error("tuple patterns are irrefutable and do not take an else branch");
        }
        return patternMatchExpr;
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
ASTNode* Parser::parseStaticDeclaration(
        ASTAllocator& allocator,
        AccessSpecifier specifier,
        bool comptime
) {
    auto* static_token = token;
    if(static_token->type != TokenType::Identifier || static_token->value != chem::string_view("static")) {
        return nullptr;
    }
    token++;
    bool is_mut = false;
    if(token->type == TokenType::MutKw) {
        is_mut = true;
        token++;
    }
    // `static` and `static mut` use the ordinary variable declaration grammar
    // after the contextual keyword.
    return parseVarInitializationTokens(
        allocator, specifier, true, false, true, comptime, true, is_mut
    );
}
