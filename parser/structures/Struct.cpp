// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"

StructMember* Parser::parseStructMember(ASTAllocator& allocator) {

    auto constId = consumeWSOfType(TokenType::ConstKw);
    if(!constId) {
        auto varId = consumeWSOfType(TokenType::VarKw);
        if(!varId) {
            return nullptr;
        }
    }

    auto identifier = consumeIdentifierOrKeyword();
    if(!identifier) {
        return nullptr;
    }

    auto member = new (allocator.allocate<StructMember>()) StructMember(allocate_view(allocator, identifier->value), nullptr, nullptr, parent_node, 0, constId != nullptr, AccessSpecifier::Public);

    if(!consumeWSOfType(TokenType::ColonSym)) {
        error("expected a colon symbol after the identifier");
    }

    auto type = parseType(allocator);
    if(type) {
        member->type = type;
    }


    if(consumeToken(TokenType::EqualSym)) {
        auto value = parseExpression(allocator);
        if(value) {
            member->defValue = value;
        } else {
            error("expected a value after equal symbol for member initialization");
        }
    }

    return member;

}

UnnamedStruct* Parser::parseUnnamedStruct(ASTAllocator& allocator, AccessSpecifier specifier) {

    if(consumeWSOfType(TokenType::StructKw)) {

        auto decl = new (allocator.allocate<UnnamedStruct>()) UnnamedStruct("", parent_node, 0, specifier);

//        if(consumeToken(TokenType::ColonSym)) {
//            do {
//                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
//                auto type = parseLinkedOrGenericType(allocator);
//                if(!type) {
//                    return decl;
//                }
//                decl->inherited.emplace_back(new InheritedType(type, in_spec));
//            } while(consumeToken(TokenType::CommaSym));
//        }
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return decl;
        }

        do {
            consumeNewLines();
            if(parseVariableMemberInto(decl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return decl;
        }
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            decl->name = allocate_view(allocator, id->value);
        } else {
            error("expected an identifier after the '}' for anonymous struct definition");
            return decl;
        }
        return decl;
    } else {
        return nullptr;
    }

}

bool Parser::parseVariableMemberInto(VariablesContainer* decl, ASTAllocator& allocator, AccessSpecifier specifier) {
    switch(token->type) {
        case TokenType::VarKw:
        case TokenType::ConstKw: {
            auto member = parseStructMember(allocator);
            if (member) {
                annotate(member);
                decl->variables[member->name] = member;
                return true;
            }
            return false;
        }
        case TokenType::Annotation:
            return parseAnnotation(allocator);
        case TokenType::StructKw:{
            auto unnamedStruct = parseUnnamedStruct(allocator, specifier);
            if(unnamedStruct) {
                annotate(unnamedStruct);
                decl->variables[unnamedStruct->name] = unnamedStruct;
                return true;
            }
            return false;
        }
        case TokenType::UnionKw:{
            auto unionStructure = parseUnnamedUnion(allocator, specifier);
            if(unionStructure) {
                annotate(unionStructure);
                decl->variables[unionStructure->name] = unionStructure;
                return true;
            }
            return false;
        }
        default:
            return false;
    }
}

bool Parser::parseVariableAndFunctionInto(MembersContainer* decl, ASTAllocator& allocator, AccessSpecifier specifier) {
    if(parseVariableMemberInto(decl, allocator, specifier)) return true;
    auto funcDecl = parseFunctionStructureTokens(allocator, specifier, true);
    if(funcDecl) {
        annotate(funcDecl);
        // TODO this maybe a generic declaration
        decl->insert_multi_func(allocator, (FunctionDeclaration*) funcDecl);
        return true;
    }
    return false;
}

ASTNode* Parser::parseStructStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    if(consumeWSOfType(TokenType::StructKw)) {

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        const auto decl = new (allocator.allocate<StructDefinition>()) StructDefinition(loc_id(allocator, identifier), parent_node, loc_single(identifier), specifier);
        annotate(decl);

        std::vector<GenericTypeParameter*> gen_params;

        auto prev_parent_node = parent_node;
        parent_node = decl;
        parseGenericParametersList(allocator, gen_params);

        ASTNode* final_decl = decl;

        if(!gen_params.empty()) {

            const auto gen_decl = new (allocator.allocate<GenericStructDecl>()) GenericStructDecl(
                decl, parent_node, loc_single(identifier)
            );

            gen_decl->generic_params = std::move(gen_params);

            final_decl = gen_decl;

        }

        if(consumeToken(TokenType::ColonSym)) {
            do {
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return final_decl;
                }
                decl->inherited.emplace_back(type, in_spec);
            } while(consumeToken(TokenType::CommaSym));
        }
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return final_decl;
        }

        do {
            consumeNewLines();
            if(parseVariableAndFunctionInto(decl, allocator, AccessSpecifier::Public)) {
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return final_decl;
        }

        parent_node = prev_parent_node;

        return final_decl;

    } else {
        return nullptr;
    }
}