// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
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

    readWhitespace();

    auto identifier = consumeIdentifierOrKeyword();
    if(!identifier) {
        return nullptr;
    }

    auto member = new (allocator.allocate<StructMember>()) StructMember(identifier->value.str(), nullptr, nullptr, parent_node, 0, constId != nullptr, AccessSpecifier::Public);

    readWhitespace();

    if(!consumeWSOfType(TokenType::ColonSym)) {
        error("expected a colon symbol after the identifier");
    }

    auto type = parseType(allocator);
    if(type) {
        member->type = type;
    }

    readWhitespace();

    if(consumeToken(TokenType::EqualSym)) {
        readWhitespace();
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

        lexWhitespaceToken();
        if(consumeToken(TokenType::ColonSym)) {
            do {
                lexWhitespaceToken();
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                readWhitespace();
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return decl;
                }
                decl->inherited.emplace_back(new InheritedType(type, in_spec));
                lexWhitespaceToken();
            } while(consumeToken(TokenType::CommaSym));
        }
        lexWhitespaceToken();
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return decl;
        }

        do {
            lexWhitespaceAndNewLines();
            if(parseVariableMemberInto(decl, allocator, AccessSpecifier::Public)) {
                lexWhitespaceToken();
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);
        lexWhitespaceToken();

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return decl;
        }
        if(lexWhitespaceToken()) {
            auto id = consumeIdentifierOrKeyword();
            if(id) {
                decl->name = id->value.str();
            } else {
                error("expected an identifier after the '}' for anonymous struct definition");
                return decl;
            }
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
        case TokenType::SingleLineComment: {
            auto comment = parseSingleLineComment(allocator);
            if (comment) {
                // TODO store comments somewhere
                return true;
            }
            return false;
        }
        case TokenType::MultiLineComment: {
            auto multilineComment = parseMultiLineComment(allocator);
            if (multilineComment) {
                // TODO store multiline comments somewhere
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
        decl->insert_multi_func(funcDecl);
        return true;
    }
    return false;
}

StructDefinition* Parser::parseStructStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier) {
    if(consumeWSOfType(TokenType::StructKw)) {

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        auto decl = new (allocator.allocate<StructDefinition>()) StructDefinition(loc_id(allocator, identifier), parent_node, 0, specifier);

        annotate(decl);

        auto prev_parent_node = parent_node;
        parent_node = decl;

        lexWhitespaceToken();
        parseGenericParametersList(allocator, decl->generic_params);
        lexWhitespaceToken();
        if(consumeToken(TokenType::ColonSym)) {
            do {
                lexWhitespaceToken();
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                readWhitespace();
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return decl;
                }
                decl->inherited.emplace_back(new InheritedType(type, in_spec));
                lexWhitespaceToken();
            } while(consumeToken(TokenType::CommaSym));
        }
        lexWhitespaceToken();
        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return decl;
        }

        do {
            lexWhitespaceAndNewLines();
            if(parseVariableAndFunctionInto(decl, allocator, AccessSpecifier::Public)) {
                lexWhitespaceToken();
                consumeToken(TokenType::SemiColonSym);
            } else {
                break;
            }
        } while(token->type != TokenType::RBrace);

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return decl;
        }

        parent_node = prev_parent_node;

        return decl;
    } else {
        return nullptr;
    }
}