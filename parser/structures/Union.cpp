// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/UnionDef.h"
#include "ast/structures/UnnamedUnion.h"

UnnamedUnion* Parser::parseUnnamedUnion(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeWSOfType(TokenType::UnionKw)) {


        auto decl = new (allocator.allocate<UnnamedUnion>()) UnnamedUnion("", parent_node, 0, specifier);

        annotate(decl);

        lexWhitespaceToken();
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return decl;
        }

        auto prev_parent_node = parent_node;
        parent_node = decl;
        do {
            lexWhitespaceAndNewLines();
            if(parseVariableMemberInto(decl, allocator, AccessSpecifier::Public)) {
                lexWhitespaceToken();
                lexOperatorToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        parent_node = prev_parent_node;

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for union block");
            return decl;
        }
        if(lexWhitespaceToken()) {
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                decl->name = id->value;
            } else {
                error("expected an identifier after the '}' for anonymous union definition");
                return decl;
            }
        }
        return decl;
    } else {
        return nullptr;
    }

}

UnionDef* Parser::parseUnionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeWSOfType(TokenType::UnionKw)) {

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        auto decl = new (allocator.allocate<UnionDef>()) UnionDef(loc_id(identifier), parent_node, 0, specifier);

        lexWhitespaceToken();
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for union block");
            return decl;
        }

        do {
            lexWhitespaceAndNewLines();
            if(parseVariableAndFunctionInto(decl, allocator, AccessSpecifier::Public)) {
                lexWhitespaceToken();
                lexOperatorToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        lexWhitespaceToken();

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for union block");
            return decl;
        }

        return decl;
    } else {
        return nullptr;
    }

}