// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/GenericVariantDecl.h"
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

const auto GENv2 = false;

ASTNode* Parser::parseVariantStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    if(consumeWSOfType(TokenType::VariantKw)) {

        auto id = consumeIdentifierOrKeyword();
        if (!id) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        const auto decl = new (allocator.allocate<VariantDefinition>()) VariantDefinition(loc_id(allocator, id), parent_node, 0, specifier);

        auto prev_parent_type = parent_node;
        parent_node = decl;

        annotate(decl);

        std::vector<GenericTypeParameter*> gen_params;

        parseGenericParametersList(allocator, gen_params);

        ASTNode* finalDecl = decl;

        if(GENv2 && !gen_params.empty()) {

            const auto gen_decl = new (allocator.allocate<GenericVariantDecl>()) GenericVariantDecl(
                decl, parent_node, loc_single(id)
            );

            gen_decl->generic_params = std::move(gen_params);

            finalDecl = gen_decl;

        } else {
            decl->generic_params = std::move(gen_params);
        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return finalDecl;
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
            return finalDecl;
        }

        return finalDecl;
    } else {
        return nullptr;
    }
}