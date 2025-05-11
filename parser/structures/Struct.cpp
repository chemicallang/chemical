// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/If.h"

StructMember* Parser::parseStructMember(ASTAllocator& allocator) {

    const auto is_const = consumeToken(TokenType::ConstKw);
    if(!is_const && !consumeToken(TokenType::VarKw)) {
        return nullptr;
    }

    auto identifier = consumeIdentifierOrKeyword();
    if(!identifier) {
        return nullptr;
    }

    auto member = new (allocator.allocate<StructMember>()) StructMember(allocate_view(allocator, identifier->value), nullptr, nullptr, parent_node, loc_single(identifier), is_const, AccessSpecifier::Public);
    annotate(member);

    if(!consumeToken(TokenType::ColonSym)) {
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

    if(consumeToken(TokenType::StructKw)) {

        auto decl = new (allocator.allocate<UnnamedStruct>()) UnnamedStruct("", parent_node, loc_single(token), specifier);
        annotate(decl);

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return decl;
        }

        do {
            consumeNewLines();
            if(parseContainerMembersInto(decl, allocator, AccessSpecifier::Public)) {
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
        }
        return decl;
    } else {
        return nullptr;
    }

}

IfStatement* parseMemberIfStatement(Parser& parser, ASTAllocator& allocator, AccessSpecifier specifier);

std::optional<Scope> parseMemberBraceBlockOrValueNode(Parser& parser, ASTAllocator& allocator, const std::string_view& forThing, AccessSpecifier specifier);

UnsafeBlock* parseMemberUnsafeBlock(Parser& parser, ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& tok = *parser.token;
    if(tok.type == TokenType::UnsafeKw) {
        parser.token++;
        auto unsafe = new (allocator.allocate<UnsafeBlock>()) UnsafeBlock(parser.parent_node, parser.loc_single(tok));
        auto block = parseMemberBraceBlockOrValueNode(parser, allocator, "unsafe_block", specifier);
        if(block.has_value()) {
            unsafe->scope = std::move(block.value());
        } else {
            parser.error("expected a braced block after 'unsafe' keyword");
            return nullptr;
        }
        return unsafe;
    } else {
        return nullptr;
    }
}

ASTNode* parseNestedLevelMemberStatementTokens(Parser& parser, ASTAllocator& fn_allocator, AccessSpecifier specifier) {
    // struct members need to be allocated globally
    // because they are part of struct prototype, which should always be allocated globally
    // because structs can be used with imported public generic structs, internal structs would need to be
    // declared in another module
    auto& allocator = parser.global_allocator;
    switch(parser.token->type) {
        case TokenType::VarKw:
        case TokenType::ConstKw:
            return (ASTNode*) parser.parseStructMember(allocator);
        case TokenType::UnsafeKw:
            return (ASTNode*) parseMemberUnsafeBlock(parser, allocator, specifier);
        case TokenType::AliasKw:
            return (ASTNode*) parser.parseAliasStatement(allocator, specifier);
        case TokenType::HashMacro:
            return parser.parseMacroNode(allocator);
        case TokenType::ComptimeKw:
            return (ASTNode*) parser.parseComptimeBlock(allocator);
        case TokenType::IfKw:
            return (ASTNode*) parseMemberIfStatement(parser, allocator, specifier);
        case TokenType::TypeKw:
            return (ASTNode*) parser.parseTypealiasStatement(allocator, specifier);
        case TokenType::StructKw:{
            return parser.parseUnnamedStruct(allocator, specifier);
        }
        case TokenType::UnionKw:{
            return parser.parseUnnamedUnion(allocator, specifier);
        }
        case TokenType::FuncKw: {
            return parser.parseFunctionStructureTokens(fn_allocator, specifier, true, false);
        }
        default:
            return nullptr;
    }
}

void parseNestedLevelMultipleMemberStatementsTokens(Parser& parser, ASTAllocator& allocator, std::vector<ASTNode*>& nodes, AccessSpecifier specifier) {
    while(true) {
        parser.consumeNewLines();
        auto stmt = parseNestedLevelMemberStatementTokens(parser, allocator, specifier);
        if(stmt) {
            nodes.emplace_back(stmt);
        } else {
            if(!parser.parseAnnotation(allocator)) {
                break;
            }
        }
        parser.consumeToken(TokenType::SemiColonSym);
    }
}

std::optional<Scope> parseMemberBraceBlockOrValueNode(Parser& parser, ASTAllocator& allocator, const std::string_view& forThing, AccessSpecifier specifier) {

    // whitespace and new lines
    parser.consumeNewLines();

    // starting brace
    auto lb = parser.consumeOfType(TokenType::LBrace);
    if (!lb) {
        auto nested_stmt = parseNestedLevelMemberStatementTokens(parser, allocator, specifier);
        if (nested_stmt) {
            parser.consumeNewLines();
            if (parser.consumeToken(TokenType::SemiColonSym)) {
                parser.consumeNewLines();
            }
            return Scope{{nested_stmt}, parser.parent_node, nested_stmt->encoded_location()};
        }
        return std::nullopt;
    }

    Scope scope(parser.parent_node, 0);

    // multiple statements
    parseNestedLevelMultipleMemberStatementsTokens(parser, allocator, scope.nodes, specifier);

    // ending brace
    auto rb = parser.consumeOfType(TokenType::RBrace);
    if (!rb) {
        parser.error() << "expected a closing brace '}' for [" << forThing << "]";
        return scope;
    }

    scope.set_encoded_location(parser.loc(lb->position, rb->position));

    return scope;

}

std::optional<std::pair<Value*, Scope>> parseMemberIfExprAndBlock(Parser& parser, ASTAllocator& allocator, AccessSpecifier specifier) {

    auto lp = parser.consumeOfType(TokenType::LParen);
    if (!lp) {
        parser.error("expected a starting parenthesis ( when lexing a if block");
        return std::nullopt;
    }

    auto expr = parser.parseExpression(allocator);
    if(!expr) {
        parser.error("expected a conditional expression when lexing a if block");
        return std::nullopt;
    }

    parser.consumeNewLines();

    if (!parser.consumeToken(TokenType::RParen)) {
        parser.error("expected a ending parenthesis ) when lexing a if block");
        return std::pair { expr, Scope { parser.parent_node, parser.loc_single(lp) } };
    }

    auto blk = parseMemberBraceBlockOrValueNode(parser, allocator, "if", specifier);
    if(blk.has_value()) {
        return std::pair { expr, std::move(blk.value()) };
    } else {
        parser.error("expected a brace block when lexing a brace block");
        return std::pair { expr, Scope { parser.parent_node, parser.loc_single(lp) } };
    }

}

IfStatement* parseMemberIfStatement(Parser& parser, ASTAllocator& allocator, AccessSpecifier specifier) {

    auto& first = *parser.token;
    if(first.type != TokenType::IfKw) {
        return nullptr;
    }

    parser.token++;

    auto statement = new (allocator.allocate<IfStatement>()) IfStatement(nullptr, parser.parent_node, false, parser.loc_single(first));

    auto exprBlock = parseMemberIfExprAndBlock(parser, allocator, specifier);
    if(exprBlock.has_value()) {
        auto& exprBlockValue = exprBlock.value();
        statement->condition = exprBlockValue.first;
        statement->ifBody = std::move(exprBlockValue.second);
    } else {
        return statement;
    }

    // lex whitespace
    parser.consumeNewLines();

    // keep lexing else if blocks until last else appears
    while (parser.consumeToken(TokenType::ElseKw)) {
        parser.consumeNewLines();
        if(parser.consumeToken(TokenType::IfKw)) {
            auto exprBlock2 = parseMemberIfExprAndBlock(parser, allocator, specifier);
            if(exprBlock2.has_value()) {
                statement->elseIfs.emplace_back(std::move(exprBlock2.value()));
            } else {
                return statement;
            }
        } else {
            auto block = parseMemberBraceBlockOrValueNode(parser, allocator, "else", specifier);
            if(block.has_value()) {
                statement->elseBody = std::move(block.value());
            } else {
                parser.error("expected a brace block after the else while lexing an if statement");
                return statement;
            }
            return statement;
        }
    }

    return statement;

}

bool Parser::parseContainerMembersInto(VariablesContainer* decl, ASTAllocator& allocator, AccessSpecifier specifier) {
    auto& nodes = decl->get_parsed_nodes_container();
    auto member = parseNestedLevelMemberStatementTokens(*this, allocator, specifier);
    if(member) {
        nodes.emplace_back(member);
        return true;
    } else {
        return parseAnnotation(allocator);
    }
}

ASTNode* Parser::parseStructStructureTokens(ASTAllocator& passed_allocator, AccessSpecifier specifier) {
    if(consumeToken(TokenType::StructKw)) {

        // all the structs are allocated on global allocator
        // WHY? because when used with imported public generics, the generics tend to instantiate with types
        // referencing the internal structs, which now must be declared inside another module
        // because generics don't check whether the type being used with it is valid in another module
        // once we can be sure which instantiations of generics are being used in module, we can eliminate this
        auto& allocator = global_allocator;

        auto identifier = consumeIdentifierOrKeyword();
        if (!identifier) {
            error("expected a identifier as struct name");
            return nullptr;
        }

        const auto decl = new (allocator.allocate<StructDefinition>()) StructDefinition(loc_id(allocator, identifier), parent_node, loc_single(identifier), specifier);
        annotate(decl);

        ASTNode* final_decl = decl;

        auto prev_parent_node = parent_node;
        parent_node = decl;

        if(token->type == TokenType::LessThanSym) {

            std::vector<GenericTypeParameter*> gen_params;

            parseGenericParametersList(allocator, gen_params);

            if (!gen_params.empty()) {

                const auto gen_decl = new(allocator.allocate<GenericStructDecl>()) GenericStructDecl(
                        decl, parent_node, loc_single(identifier)
                );

                decl->generic_parent = gen_decl;

                gen_decl->generic_params = std::move(gen_params);

                final_decl = gen_decl;

            }

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
            if(parseContainerMembersInto(decl, passed_allocator, AccessSpecifier::Public)) {
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