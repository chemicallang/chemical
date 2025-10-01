// Copyright (c) Chemical Language Foundation 2025.

#include "parser/Parser.h"
#include "ast/statements/Typealias.h"
#include "ast/structures/GenericTypeDecl.h"
#include "ast/base/TypeBuilder.h"
#include "ast/types/IfType.h"

std::pair<Value*, TypeLoc> parseTypeIffy(Parser& parser, ASTAllocator& allocator) {
    auto& token = parser.token;
    // assuming its a if keyword
    token++;

    if(!parser.consumeToken(TokenType::LParen)) {
        parser.error("expected a '(' after keyword 'if' for an if type");
        return {nullptr, nullptr};
    }

    parser.consumeNewLines();

    const auto expr = parser.parseExpression(allocator, false, false);
    if(!expr) {
        parser.error("expected an expression after '(' in if type");
        return {nullptr, nullptr};
    }

    parser.consumeNewLines();

    if(!parser.consumeToken(TokenType::RParen)) {
        parser.error("expected a ')' after keyword 'if' for an if type");
        return {expr, nullptr};
    }

    const auto thenType = parser.parseTypeLoc(allocator);

    if(!thenType) {
        parser.error("expected a then type for if type");
        return {expr, nullptr};
    }

    return {expr, thenType};

}

IfType* parseIfType(Parser& parser, ASTAllocator& allocator) {
    auto& token = parser.token;
    // assuming its a if keyword
    token++;

    const auto ifType = new (allocator.allocate<IfType>()) IfType(
        nullptr, nullptr, nullptr
    );

    auto iffy = parseTypeIffy(parser, allocator);
    // save the condition first
    ifType->condition = iffy.first;
    // check for any error
    if(!iffy.second) {
        return ifType;
    }
    ifType->thenType = iffy.second;

    // parse all else ifs
    while(true) {
        if(!parser.consumeToken(TokenType::ElseKw)) {
            parser.error("expected an 'else' keyword for an if type");
            return ifType;
        }
        if(parser.consumeToken(TokenType::IfKw)) {
            auto elseIffy = parseTypeIffy(parser, allocator);
            if(!elseIffy.first) {
                // probably an error occurred
                return ifType;
            }
            ifType->elseIfs.emplace_back(elseIffy);
        } else {
            break;
        }
    }

    // parse the else type (always must exist)
    const auto elseType = parser.parseTypeLoc(allocator);
    if(!elseType) {
        parser.error("expected an else type for if type");
        return ifType;
    }
    ifType->elseType = elseType;

    return ifType;

}

ASTNode* Parser::parseTypealiasStatement(ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *token;
    if(tok.type == TokenType::TypeKw) {
        token++;

        auto id = consumeIdentifierOrKeyword();
        if(!id) {
            unexpected_error("expected a type for typealias statement");
            return nullptr;
        }

        const auto loc = loc_single(tok);
        const auto alias = new (allocator.allocate<TypealiasStatement>()) TypealiasStatement(
                loc_id(allocator, id),
                { (BaseType*) typeBuilder.getVoidType(), ZERO_LOC },
                parent_node,
                loc,
                specifier
        );
        annotate(alias);

#ifdef LSP_BUILD
        id->linked = alias;
#endif

        ASTNode* final_decl = alias;

        if(token->type == TokenType::LessThanSym) {

            std::vector<GenericTypeParameter*> gen_params;
            parseGenericParametersList(allocator, gen_params);

            if (!gen_params.empty()) {

                const auto gen_decl = new(allocator.allocate<GenericTypeDecl>()) GenericTypeDecl(
                        alias, parent_node, loc
                );

                final_decl = gen_decl;

                gen_decl->generic_params = std::move(gen_params);

            }

        }

        if(!consumeToken(TokenType::EqualSym)) {
            unexpected_error("expected '=' after the type tokens");
        }

        // parse the type (which can be an if expression)
        switch(token->type) {
            case TokenType::IfKw: {
                const auto saved_loc = loc_single(token);
                // user is going to write an if type
                // lets parse that
                const auto ifType = parseIfType(*this, allocator);
                if (ifType) {
                    alias->actual_type = {ifType, saved_loc};
                } else {
                    error("expected an if type after '='");
                }
                break;
            }
            default: {
                // parsing the actual type
                const auto type = parseTypeLoc(allocator);
                if (type) {

                    const auto final_type = parseExpressionType(allocator, type);
                    alias->actual_type = {final_type, type.getLocation()};

                } else {
                    error("expected a type after '='");
                }
                break;
            }
        }

        return final_decl;
    } else {
        return nullptr;
    }
}