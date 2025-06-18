// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/structures/FunctionParam.h"
#include "ast/types/LinkedType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/VoidType.h"
#include "ast/structures/GenericTypeParameter.h"
#include "ast/structures/FunctionDeclaration.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/InitBlock.h"
#include "ast/structures/GenericFuncDecl.h"
#include "ast/statements/Return.h"
#include "ast/statements/DestructStmt.h"
#include "ast/values/NullValue.h"
#include "ast/base/TypeBuilder.h"

ReturnStatement* Parser::parseReturnStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ReturnKw) {
        token++;
        auto stmt = new (allocator.allocate<ReturnStatement>()) ReturnStatement(nullptr, parent_node, loc_single(tok));
        auto expr = parseExpressionOrArrayOrStruct(allocator);
        if(expr) {
            stmt->value = expr;
        }
        return stmt;
    } else {
        return nullptr;
    }
}

InitBlock* Parser::parseConstructorInitBlock(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::InitKw) {
        token++;
        auto init = new (allocator.allocate<InitBlock>()) InitBlock(parent_node, loc_single(tok));
        if(token->type == TokenType::LBrace) {
            token++;
        } else {
            unexpected_error("expected '{' for beginning of init block");
        }
        while(true) {
            consumeNewLines();
            const auto id = consumeIdentifierOrKeyword();
            if(id) {
                const auto has_lparen = token->type == TokenType::LParen;
                if(has_lparen) {
                    token++;
                    consumeNewLines();
                } else if(token->type == TokenType::EqualSym) {
                    token++;
                } else {
                    unexpected_error("expected '(' or '=' for initializing the init member");
                }
                const auto value = parseExpression(allocator, true);
                if(value) {
                    init->initializers[allocate_view(allocator, id->value)] = { value };
                } else {
                    unexpected_error("expected an expression for initializing init member");
                }
                if(has_lparen) {
                    consumeNewLines();
                    if(token->type == TokenType::RParen) {
                        token++;
                    } else {
                        unexpected_error("expected ')' for init member");
                    }
                }
                if(token->type == TokenType::SemiColonSym) {
                    token++;
                }
            } else {
                break;
            }
        }
        if(token->type == TokenType::RBrace) {
            token++;
        } else {
            unexpected_error("expected '}' for ending the init block");
        }
        return init;
    } else {
        return nullptr;
    }
}

UnsafeBlock* Parser::parseUnsafeBlock(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::UnsafeKw) {
        token++;
        auto unsafe = new (allocator.allocate<UnsafeBlock>()) UnsafeBlock(parent_node, loc_single(tok));
        auto block = parseBraceBlock("unsafe_block", unsafe, allocator);
        if(block.has_value()) {
            unsafe->scope = std::move(block.value());
        } else {
            unexpected_error("expected a braced block after 'unsafe' keyword");
            return nullptr;
        }
        return unsafe;
    } else {
        return nullptr;
    }
}

DestructStmt* fix_destruct(DestructStmt* stmt, ASTAllocator& allocator) {
    if(!stmt->identifier) {
        stmt->identifier = new(allocator.allocate<NullValue>()) NullValue(nullptr, ZERO_LOC);
    }
    return stmt;
}

DestructStmt* Parser::parseDestructStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::DestructKw) {
        token++;
        auto stmt = new (allocator.allocate<DestructStmt>()) DestructStmt(nullptr, nullptr, false, parent_node, loc_single(tok));
        if(consumeToken(TokenType::LBracket)) {
            stmt->is_array = true;
            auto value = parseAccessChainOrValue(allocator);
            if(value) {
                stmt->array_value = value;
            }
            if(!consumeToken(TokenType::RBracket)) {
                unexpected_error("expected a ']' after the access chain value");
                return fix_destruct(stmt, allocator);
            }
        }
        auto value = parseAccessChainOrValue(allocator);
        if(value) {
            stmt->identifier = value;
        } else {
            unexpected_error("expected a pointer value for the destruct statement");
            return fix_destruct(stmt, allocator);
        }
        return stmt;
    } else {
        return nullptr;
    }
}

bool Parser::parseParameterList(
        ASTAllocator& allocator,
        std::vector<FunctionParam*>& parameters,
        bool optionalTypes,
        bool defValues,
        bool lexImplicitParams,
        bool variadicParam
) {
    unsigned int index = parameters.size();
    do {
        consumeNewLines();
        if(lexImplicitParams) {
            auto ampersand = consumeOfType(TokenType::AmpersandSym);
            if (ampersand) {
                auto is_mutable = consumeToken(TokenType::MutKw); // optional mut keyword
                auto id = consumeIdentifierOrKeyword();
                if (id) {
                    const auto linkedType = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, id->value), nullptr);
#ifdef LSP_BUILD
                    id->linked = linkedType;
#endif
                    const auto ref_to_linked  = new (allocator.allocate<ReferenceType>()) ReferenceType(linkedType, is_mutable);
                    auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, id->value), TypeLoc(ref_to_linked, loc_single(id)), index, nullptr, true, parent_node, loc(ampersand, id));
                    parameters.emplace_back(param);
                    index++;
                    continue;
                } else {
                    unexpected_error("expected a identifier right after '&' in the first function parameter as a 'self' parameter");
                    return false;
                }
            }
        }
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            if(consumeToken(TokenType::ColonSym)) {
                const auto typeLoc = parseTypeLoc(allocator);
                auto type = typeLoc.getType();
                if(type) {
                    if(variadicParam && consumeToken(TokenType::TripleDotSym)) {
                        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, id->value), typeLoc, index, nullptr, false, parent_node, loc_single(id));
                        parameters.emplace_back(param);
#ifdef LSP_BUILD
                        id->linked = param;
#endif
                        return true;
                    }
                    Value* defValue = nullptr;
                    if(defValues) {
                        if (consumeToken(TokenType::EqualSym)) {
                            auto expr = parseExpression(allocator);
                            if(expr) {
                                defValue = expr;
                            } else {
                                unexpected_error("expected value after '=' for default value for the parameter");
                                break;
                            }
                        }
                    }
                    auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, id->value), typeLoc, index, defValue, false, parent_node, loc_single(id));
                    parameters.emplace_back(param);
#ifdef LSP_BUILD
                    id->linked = param;
#endif
                } else {
                    unexpected_error("missing a type token for the function parameter, expected type after the colon");
                    return false;
                }
            } else {
                if(optionalTypes) {
                    auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(allocate_view(allocator, id->value), nullptr, index, nullptr, false, parent_node, loc_single(id));
                    parameters.emplace_back(param);
#ifdef LSP_BUILD
                    id->linked = param;
#endif
                } else {
                    error("expected colon ':' in function parameter list after the parameter name ");
                    return false;
                }
            }
        }
        index++;
    } while(consumeToken(TokenType::CommaSym));
    return false;
}

bool Parser::parseGenericParametersList(ASTAllocator& allocator, std::vector<GenericTypeParameter*>& params) {
    if(consumeToken(TokenType::LessThanSym)) {
        unsigned int param_index = 0;
        while(true) {
            auto id = consumeIdentifierOrKeyword();
            if(!id) {
                break;
            }
            auto parameter = new (allocator.allocate<GenericTypeParameter>()) GenericTypeParameter(allocate_view(allocator, id->value), nullptr, nullptr, parent_node, param_index, loc_single(id));
            params.emplace_back(parameter);
            param_index++;

#ifdef LSP_BUILD
            id->linked = parameter;
#endif

            if(consumeToken(TokenType::ColonSym)) {
                auto type = parseTypeLoc(allocator);
                if(type) {
                    parameter->at_least_type = type;
                } else {
                    error("expected a type after ':' in generic parameter list");
                    return true;
                }
            }
            if(consumeToken(TokenType::EqualSym)) {
                auto type = parseTypeLoc(allocator);
                if(type) {
                    parameter->def_type = type;
                } else {
                    error("expected a default type after '=' in generic parameter list");
                    return true;
                }
            }
            if(!consumeToken(TokenType::CommaSym)) {
                break;
            }
        }
        if(!consumeToken(TokenType::GreaterThanSym)) {
            error("expected a '>' for ending the generic parameters list");
            return true;
        }
        return true;
    } else {
        return false;
    }
}

ASTNode* Parser::parseFunctionStructureTokens(ASTAllocator& passed_allocator, AccessSpecifier specifier, bool member, bool allow_extensions) {

    if(!consumeToken(TokenType::FuncKw)) {
        return nullptr;
    }

    // by default comptime functions are allocated using job allocator
    // because they can be called indirectly in different modules
    // so we retain them, throughout executable
    bool is_comptime = false;
    for(auto& annot : annotations) {
        if(annot.name == "comptime") {
            is_comptime = true;
        }
    }

    // comptime functions must be allocated on global allocator
    // they can be called from external modules, without being public
    auto& body_allocator = is_comptime ? global_allocator : passed_allocator;
    // this allocator is for the prototype
    // all member functions (inside structs/variants), must allocate prototypes on global allocator
    // because they can be used with imported public generic structs, which may declare them in another modules (not because of usage)
    // because generics don't check whether the type being used with it is valid in another module
    // once we can be sure which instantiations of generics are being used in module, we can eliminate this check
    auto& allocator = member ? global_allocator : body_allocator;

    const auto decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(LocatedIdentifier(""), { typeBuilder.getVoidType(), ZERO_LOC }, false, parent_node, 0, specifier, false);
    annotate(decl);

    ASTNode* final_node = decl;

    if(token->type == TokenType::LessThanSym) {

        std::vector<GenericTypeParameter*> gen_params;

        parseGenericParametersList(allocator, gen_params);

        for(auto param : gen_params) {
            param->set_parent(decl);
        }

        if(!gen_params.empty()) {

            const auto gen_decl = new (allocator.allocate<GenericFuncDecl>()) GenericFuncDecl(decl, parent_node, 0);

            gen_decl->generic_params = std::move(gen_params);

            decl->generic_parent = gen_decl;

            final_node = gen_decl;

        }

    }

    if(allow_extensions && consumeToken(TokenType::LParen)) {

        const auto receiverParam = new (allocator.allocate<FunctionParam>()) FunctionParam("", nullptr, 0, nullptr, false, decl, loc_single(token));

        auto id = consumeIdentifierOrKeyword();
        if(id) {
            receiverParam->name = allocate_view(allocator, id->value);
#ifdef LSP_BUILD
            id->linked = receiverParam;
#endif
        } else {
            error("expected identifier for receiver in extension function after '('");
            return decl;
        }
        if(!consumeToken(TokenType::ColonSym)) {
            error("expected ':' in extension function after identifier for receiver");
            return decl;
        }
        const auto typeLoc = parseTypeLoc(allocator);
        auto type = typeLoc.getType();
        if(type) {
            receiverParam->type = typeLoc;
            decl->params.emplace_back(receiverParam);
            // set the function is extension
            decl->setIsExtension(true);
        } else {
            error("expected type after ':' in extension function for receiver");
            return decl;
        }
        if(!consumeToken(TokenType::RParen)) {
            error("expected ')' in extension function after receiver");
            return decl;
        }
    }

    auto name = consumeIdentifierOrKeyword();

    if(!name) {
        error("function name is missing after the keyword 'func'");
        return decl;
    }

#ifdef LSP_BUILD
    name->linked = (ASTNode*) decl;
#endif

    const auto location = loc_single(name);
    decl->set_encoded_location(location);
    decl->set_identifier(loc_id(allocator, name));
    if(decl->generic_parent) {
        decl->generic_parent->set_encoded_location(location);
    }

    if(!consumeToken(TokenType::LParen)) {
        error("expected a starting parenthesis ( in a function signature");
        return final_node;
    }

    // inside the block allow return statements
    auto prev_parent_node = parent_node;
    parent_node = decl;

    auto isVariadic = parseParameterList(allocator, decl->params);
    decl->setIsVariadic(isVariadic);

    consumeNewLines();

    if(!consumeToken(TokenType::RParen)) {
        error("expected a closing parenthesis ) when ending a function signature");
        return final_node;
    }

    auto& tok = *token;
    if(tok.type == TokenType::ColonSym) {
        token++;
        auto type = parseTypeLoc(allocator);
        if(type) {
            decl->returnType = type;
        } else {
            error("expected a return type for function after ':'");
            return final_node;
        }
    } else {
        decl->returnType = { typeBuilder.getVoidType(), location };
    }

    auto block = parseBraceBlock("function", decl, body_allocator);
    if(block.has_value()) {
        decl->body.emplace(block->parent(), block->encoded_location());
        decl->body->nodes = std::move(block->nodes);
    }
    parent_node = prev_parent_node;

    return final_node;

}