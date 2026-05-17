// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/VariantDefinition.h"
#include "ast/structures/GenericVariantDecl.h"
#include "ast/structures/VariantMember.h"
#include "ast/structures/VariantMemberParam.h"

VariantMember* Parser::parseVariantMember(ASTAllocator& allocator, VariantDefinition* definition) {
    auto id = consumeIdentifierOrKeyword();
    if(id) {

        auto member = new (allocator.allocate<VariantMember>()) VariantMember(allocate_view(allocator, id->value), definition, loc_single(id));
        annotate(member);

#ifdef LSP_BUILD
        id->linked = member;
#endif

        if(consumeToken(TokenType::LParen)) {

            unsigned int index = 0;
            while(true) {

                auto paramId = consumeIdentifierOrKeyword();
                if(paramId) {

                    auto name_view = allocate_view(allocator, paramId->value);
                    auto param = new (allocator.allocate<VariantMemberParam>()) VariantMemberParam(name_view, index, false, nullptr, nullptr, member, loc_single(paramId));
                    member->values[name_view] = param;


#ifdef LSP_BUILD
                    paramId->linked = param;
#endif

                    if(!consumeToken(TokenType::ColonSym)) {
                        unexpected_error("expected ':' after the variant member parameter");
                    }

                    auto type = parseTypeLoc(allocator);
                    if(type) {
                        param->type = type;
                    } else {
                        unexpected_error("expected a type for variant member parameter");
                        param->type = { (BaseType*) typeBuilder.getVoidType(), ZERO_LOC };
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

bool Parser::parseAnyVariantMember(ASTAllocator& allocator, ASTAllocator& body_allocator, VariantDefinition* def, AccessSpecifier specifier, bool comptime) {
    auto annotation = parseAnnotation(allocator);
    if(annotation) {
        return true;
    }
    switch(token->type) {
        case TokenType::ComptimeKw:
            if(comptime) {
                error("already inside comptime context");
            }
            token++;
            if(token->type == TokenType::LBrace) {
                const auto blk = (ASTNode*) parseComptimeBlockNoKw(allocator);
                def->get_parsed_nodes_container().emplace_back(blk);
                return true;
            } else {
                return parseAnyVariantMember(allocator, body_allocator, def, specifier, true);
            }
        case TokenType::ImplKw: {
            const auto implNode = parseImplTokens(allocator, body_allocator, specifier);
            if (implNode) {
                def->get_parsed_nodes_container().emplace_back(implNode);
                return true;
            } else {
                error("couldn't parse impl declaration");
            }
        }
        case TokenType::FuncKw: {
            const auto func = parseFunctionStructureTokens(allocator, body_allocator, specifier, false, comptime);
            if(func) {
                def->get_parsed_nodes_container().emplace_back(func);
                return true;
            }
            break;
        }
        default:
            auto variantMember = parseVariantMember(allocator, def);
            if(variantMember) {
//                def->get_parsed_nodes_container().emplace_back(variantMember);
                if(!def->insert_variable(variantMember)) {
                    error() << "couldn't insert variable with name '" << variantMember->name << "' because a member with same name already exists";
                }
                return true;
            }
    }
    return false;
}

ASTNode* Parser::parseVariantStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    if(consumeToken(TokenType::VariantKw)) {

        auto id = consumeIdentifierOrKeyword();
        if (!id) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        const auto decl = new (allocator.allocate<VariantDefinition>()) VariantDefinition(loc_id(allocator, id), parent_node, loc_single(id), specifier);
        annotate(decl);

#ifdef LSP_BUILD
        id->linked = decl;
#endif

        auto prev_parent_type = parent_node;

        ASTNode* finalDecl = decl;

        if(token->type == TokenType::LessThanSym) {

            const auto gen_decl = new (allocator.allocate<GenericVariantDecl>()) GenericVariantDecl(
                    decl, prev_parent_type, loc_single(id)
            );

            parent_node = gen_decl;

            parseGenericParametersList(allocator, gen_decl->generic_params);

            decl->generic_parent = gen_decl;

            finalDecl = gen_decl;

        } else {

            parent_node = decl;

        }

        bool use_allocator = false;
        if (decl->is_body_retained()) {
            use_allocator = true;
        } else if (finalDecl->kind() == ASTNodeKind::GenericVariantDecl) {
            use_allocator = true;
            decl->set_is_body_retained(true);
        }

        // bodies of functions will be allocated on the passed allocator only if
        // containing is a generic struct, otherwise we can allocate on mod_allocator
        auto& body_allocator = use_allocator ? allocator : mod_allocator;

        // parsing the inheritance list
        if(consumeToken(TokenType::ColonSym)) {
            do {
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                const auto typeLoc = loc_single(token);
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return finalDecl;
                }
                decl->inherited.emplace_back(TypeLoc{type, typeLoc}, in_spec);
            } while(consumeToken(TokenType::CommaSym));
        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return finalDecl;
        }

        do {
            consumeNewLines();
            if(parseAnyVariantMember(allocator, body_allocator, decl, AccessSpecifier::Public, false)) {
                switch(token->type) {
                    case TokenType::SemiColonSym:
                    case TokenType::CommaSym:
                        token++;
                        break;
                    default:
                        break;
                }
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