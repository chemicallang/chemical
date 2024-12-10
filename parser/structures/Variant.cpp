// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/Variantdefinition.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"

VariantMember* Parser::parseVariantMember(ASTAllocator& allocator, VariantDefinition* definition) {
    auto id = consumeIdentifierOrKeyword();
    if(id) {
        auto member = new (allocator.allocate<VariantMember>()) VariantMember(std::string(id->value), definition, loc_single(id));
        annotate(member);
        readWhitespace();
        if(consumeToken(TokenType::LParen)) {

            unsigned int index = 0;
            while(true) {

                auto paramId = consumeIdentifierOrKeyword();
                if(paramId) {

                    readWhitespace();

                    auto param = new (allocator.allocate<VariantMemberParam>()) VariantMemberParam(std::string(paramId->value), index, false, nullptr, nullptr, member, 0);
                    member->values[std::string(paramId->value)] = param;

                    if(!consumeToken(TokenType::ColonSym)) {
                        error("expected ':' after the variant member parameter");
                    }

                    readWhitespace();

                    auto type = parseType(allocator);
                    if(type) {
                        param->type = type;
                    }

                    readWhitespace();

                    if(consumeToken(TokenType::EqualSym)) {
                        auto defValue = parseExpression(allocator);
                        if(defValue) {
                            param->def_value = defValue;
                        }
                    }

                    readWhitespace();

                    if(consumeToken(TokenType::CommaSym)) {
                        readWhitespace();
                        continue;
                    }

                    index++;
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
    auto comment = parseSingleLineComment(allocator);
    if(comment) {
        // TODO store comments somewhere
        return true;
    }
    auto multilineComment = parseMultiLineComment(allocator);
    if(multilineComment) {
        // TODO store multiline comments somewhere
        return true;
    }
    auto annotation = parseAnnotation(allocator);
    if(annotation) {
        return true;
    }
    auto funcDecl = parseFunctionStructureTokens(allocator, specifier, true);
    if(funcDecl) {
        annotate(funcDecl);
        decl->insert_multi_func(funcDecl);
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

        auto decl = new (allocator.allocate<VariantDefinition>()) VariantDefinition(loc_id(id), parent_node, 0, specifier);

        annotate(decl);

        lexWhitespaceToken();
        parseGenericParametersList(allocator, decl->generic_params);

        lexWhitespaceToken();
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return decl;
        }

        auto prev_parent_type = parent_node;
        parent_node = decl;
        do {
            lexWhitespaceAndNewLines();
            if(parseAnyVariantMember(allocator, decl, AccessSpecifier::Public)) {
                readWhitespace();
                consumeOfType(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        lexWhitespaceToken();
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