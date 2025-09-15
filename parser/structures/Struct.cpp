// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 27/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/StructDefinition.h"
#include "ast/structures/GenericStructDecl.h"
#include "ast/base/TypeBuilder.h"
#include "ast/structures/UnnamedUnion.h"
#include "ast/structures/UnnamedStruct.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/If.h"

StructMember* Parser::parseStructMember(ASTAllocator& allocator, AccessSpecifier specifier) {

    const auto is_const = consumeToken(TokenType::ConstKw);
    if(!is_const && !consumeToken(TokenType::VarKw)) {
        return nullptr;
    }

    auto identifier = consumeIdentifierOrKeyword();
    if(!identifier) {
        return nullptr;
    }

    auto member = new (allocator.allocate<StructMember>()) StructMember(allocate_view(allocator, identifier->value), { (BaseType*) typeBuilder.getVoidType(), ZERO_LOC }, nullptr, parent_node, loc_single(identifier), is_const, specifier);
    annotate(member);

#ifdef LSP_BUILD
    identifier->linked = member;
#endif

    if(!consumeToken(TokenType::ColonSym)) {
        unexpected_error("expected a colon symbol after the identifier");
    }

    auto type = parseTypeLoc(allocator);
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

        parseContainerMembersInto(decl, allocator, AccessSpecifier::Public, false);

        if(!consumeToken(TokenType::RBrace)) {
            error("expected a closing bracket '}' for struct block");
            return decl;
        }
        auto id = consumeIdentifierOrKeyword();
        if(id) {
#ifdef LSP_BUILD
            id->linked = decl;
#endif
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

ASTNode* parseAccessSpecifiedMemberStmt(Parser& parser, ASTAllocator& fn_allocator, AccessSpecifier specifier, bool comptime) {
    // struct members need to be allocated globally
    // because they are part of struct prototype, which should always be allocated globally
    // because structs can be used with imported public generic structs, internal structs would need to be
    // declared in another module
    auto& allocator = parser.global_allocator;
    switch(parser.token->type) {
        case TokenType::VarKw:
        case TokenType::ConstKw:
            return (ASTNode*) parser.parseStructMember(allocator, specifier);
        case TokenType::ComptimeKw:
            if(comptime) {
                parser.error("already inside comptime context");
            }
            parser.token++;
            if(parser.token->type == TokenType::LBrace) {
                return (ASTNode*) parser.parseComptimeBlockNoKw(allocator);
            } else {
                return parseAccessSpecifiedMemberStmt(parser, fn_allocator, specifier, true);
            }
        case TokenType::AliasKw:
            return (ASTNode*) parser.parseAliasStatement(allocator, specifier);
        case TokenType::TypeKw:
            return (ASTNode*) parser.parseTypealiasStatement(allocator, specifier);
        case TokenType::StructKw:
            return parser.parseUnnamedStruct(allocator, specifier);
        case TokenType::UnionKw:
            return parser.parseUnnamedUnion(allocator, specifier);
        case TokenType::FuncKw:
            return parser.parseFunctionStructureTokens(fn_allocator, specifier, true, false, comptime);
        default:
            return nullptr;
    }
}

ASTNode* parseMemberStmt(Parser& parser, ASTAllocator& fn_allocator, AccessSpecifier specifier, bool comptime) {
    // struct members need to be allocated globally
    // because they are part of struct prototype, which should always be allocated globally
    // because structs can be used with imported public generic structs, internal structs would need to be
    // declared in another module
    auto& allocator = parser.global_allocator;
    const auto tokenType = parser.token->type;
    switch(tokenType) {
        case TokenType::VarKw:
        case TokenType::ConstKw:
            return (ASTNode*) parser.parseStructMember(allocator, specifier);
        case TokenType::ComptimeKw:
            if(comptime) {
                parser.error("already inside comptime context");
            }
            parser.token++;
            if(parser.token->type == TokenType::LBrace) {
                return (ASTNode*) parser.parseComptimeBlockNoKw(allocator);
            } else {
                return parseAccessSpecifiedMemberStmt(parser, fn_allocator, specifier, true);
            }
        case TokenType::UnsafeKw:
            return (ASTNode*) parseMemberUnsafeBlock(parser, allocator, specifier);
        case TokenType::AliasKw:
            return (ASTNode*) parser.parseAliasStatement(allocator, specifier);
        case TokenType::HashMacro:
            return parser.parseMacroNode(allocator, CBIFunctionType::ParseMacroMemberNode);
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
            return parser.parseFunctionStructureTokens(fn_allocator, specifier, true, false, comptime);
        }
        default:
            return nullptr;
    }
}

void Parser::parseContainerMembersInto(VariablesContainer* decl, ASTAllocator& allocator, AccessSpecifier specifier, bool comptime) {
    auto& nodes = decl->get_parsed_nodes_container();
    do {
        const auto tokenType = token->type;
        switch(tokenType) {
            case TokenType::NewLine:
                token++;
                continue;
            case TokenType::PublicKw:
            case TokenType::InternalKw:
            case TokenType::ProtectedKw:
            case TokenType::PrivateKw:
                token++;
                if(token->type == TokenType::ColonSym) {
                    token++;
                    specifier = get_specifier_from(tokenType);
                    continue;
                } else {
                    const auto member = parseAccessSpecifiedMemberStmt(*this, allocator, get_specifier_from(tokenType), comptime);
                    if(member) {
                        nodes.emplace_back(member);
                        consumeToken(TokenType::SemiColonSym);
                        continue;
                    } else {
                        error("expected a member declaration after access specifier");
                        return;
                    }
                }
            default:
                break;
        }
        auto member = parseMemberStmt(*this, allocator, specifier, comptime);
        if(member) {
            nodes.emplace_back(member);
            consumeToken(TokenType::SemiColonSym);
        } else if(!parseAnnotation(allocator)) {
            return;
        }
    } while(token->type != TokenType::RBrace);
}

void parseMultipleMemberStmts(Parser& parser, ASTAllocator& allocator, std::vector<ASTNode*>& nodes, AccessSpecifier specifier, bool comptime) {
    while(true) {
        parser.consumeNewLines();
        auto stmt = parseMemberStmt(parser, allocator, specifier, comptime);
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
        auto nested_stmt = parseMemberStmt(parser, allocator, specifier, false);
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
    parseMultipleMemberStmts(parser, allocator, scope.nodes, specifier, false);

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

    auto statement = new (allocator.allocate<IfStatement>()) IfStatement(nullptr, parser.parent_node, parser.loc_single(first));

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

#ifdef LSP_BUILD
        identifier->linked = decl;
#endif

        ASTNode* final_decl = decl;

        auto prev_parent_node = parent_node;

        if(token->type == TokenType::LessThanSym) {

            const auto gen_decl = new(allocator.allocate<GenericStructDecl>()) GenericStructDecl(
                    decl, prev_parent_node, loc_single(identifier)
            );

            parent_node = gen_decl;

            parseGenericParametersList(allocator, gen_decl->generic_params);

            decl->generic_parent = gen_decl;

            final_decl = gen_decl;

        } else {

            parent_node = decl;

        }

        // parsing the inheritance list
        if(consumeToken(TokenType::ColonSym)) {
            do {
                auto in_spec = parseAccessSpecifier(AccessSpecifier::Public);
                const auto typeLoc = loc_single(token);
                auto type = parseLinkedOrGenericType(allocator);
                if(!type) {
                    return final_decl;
                }
                decl->inherited.emplace_back(TypeLoc{type, typeLoc}, in_spec);
            } while(consumeToken(TokenType::CommaSym));
        }

        if(!consumeToken(TokenType::LBrace)) {
            error("expected a '{' for struct block");
            return final_decl;
        }

        parseContainerMembersInto(decl, passed_allocator, AccessSpecifier::Public, false);

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