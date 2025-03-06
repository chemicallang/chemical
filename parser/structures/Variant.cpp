// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"

VariantMember* Parser::parseVariantMember(ASTAllocator& allocator, VariantDefinition* definition) {
    auto id = consumeIdentifierOrKeyword();
    if(id) {
        auto member = new (allocator.allocate<VariantMember>()) VariantMember(allocate_view(allocator, id->value), definition, loc_single(id));
        annotate(member);
        if(consumeToken(TokenType::LParen)) {

            unsigned int index = 0;
            while(true) {

                auto paramId = consumeIdentifierOrKeyword();
                if(paramId) {

                    auto name_view = allocate_view(allocator, paramId->value);
                    auto param = new (allocator.allocate<VariantMemberParam>()) VariantMemberParam(name_view, index, false, nullptr, nullptr, member, loc_single(paramId));
                    member->values[name_view] = param;

                    if(!consumeToken(TokenType::ColonSym)) {
                        error("expected ':' after the variant member parameter");
                    }

                    auto type = parseType(allocator);
                    if(type) {
                        param->type = type;
                    }

                    if(consumeToken(TokenType::EqualSym)) {
                        auto defValue = parseExpression(allocator);
                        if(defValue) {
                            param->def_value = defValue;
                        }
                    }

                    index++;

                    if(consumeToken(TokenType::CommaSym)) {
                        continue;
                    }

                } else {
                    break;
                }
            }

            if(!consumeToken(TokenType::RParen)) {
                error("expected a ')' after the variant member");
            }

        }
        return member;
    } else {
        return nullptr;
    }
}

bool Parser::parseAnyVariantMember(ASTAllocator& allocator, VariantDefinition* decl, AccessSpecifier specifier) {
    auto annotation = parseAnnotation(allocator);
    if(annotation) {
        return true;
    }
    auto funcDecl = parseFunctionStructureTokens(allocator, specifier, true);
    if(funcDecl) {
        annotate(funcDecl);
        // TODO this maybe a generic function declaration
        decl->insert_multi_func(allocator, (FunctionDeclaration*) funcDecl);
        return true;
    }
    auto variantMember = parseVariantMember(allocator, decl);
    if(variantMember) {
        decl->variables[variantMember->name] = variantMember;
        return true;
    }
    return false;

}

VariantDefinition* Parser::parseVariantStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    if(consumeWSOfType(TokenType::VariantKw)) {

        auto id = consumeIdentifierOrKeyword();
        if (!id) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        auto decl = new (allocator.allocate<VariantDefinition>()) VariantDefinition(loc_id(allocator, id), parent_node, 0, specifier);

        auto prev_parent_type = parent_node;
        parent_node = decl;

        annotate(decl);

        parseGenericParametersList(allocator, decl->generic_params);

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return decl;
        }

        do {
            consumeNewLines();
            if(parseAnyVariantMember(allocator, decl, AccessSpecifier::Public)) {
                consumeOfType(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        parent_node = prev_parent_type;

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return decl;
        }
        return decl;
    } else {
        return nullptr;
    }
}