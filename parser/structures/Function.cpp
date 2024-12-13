// Copyright (c) Qinetik 2024.

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
#include "ast/structures/ExtensionFunction.h"
#include "ast/structures/UnsafeBlock.h"
#include "ast/structures/InitBlock.h"
#include "ast/statements/Return.h"
#include "ast/statements/DestructStmt.h"

ReturnStatement* Parser::parseReturnStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::ReturnKw) {
        token++;
        lexWhitespaceToken();
        auto stmt = new (allocator.allocate<ReturnStatement>()) ReturnStatement(nullptr, current_func_type, parent_node, loc_single(tok));
        auto expr = parseExpression(allocator, true);
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
        readWhitespace();
        auto init = new (allocator.allocate<InitBlock>()) InitBlock({ nullptr, 0 }, parent_node, loc_single(tok));
        auto block = parseBraceBlock("init-block", init, allocator);
        if(block.has_value()) {
            init->scope = std::move(block.value());
        } else {
            error("expected a block after init");
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
        readWhitespace();
        auto unsafe = new (allocator.allocate<UnsafeBlock>()) UnsafeBlock({ nullptr, 0 }, loc_single(tok));
        auto block = parseBraceBlock("unsafe_block", unsafe, allocator);
        if(block.has_value()) {
            unsafe->scope = std::move(block.value());
        } else {
            error("expected a braced block after 'unsafe' keyword");
            return nullptr;
        }
        return unsafe;
    } else {
        return nullptr;
    }
}

DestructStmt* Parser::parseDestructStatement(ASTAllocator& allocator) {
    auto& tok = *token;
    if(tok.type == TokenType::DestructKw) {
        token++;
        readWhitespace();
        auto stmt = new (allocator.allocate<DestructStmt>()) DestructStmt(nullptr, nullptr, false, parent_node, loc_single(tok));
        if(consumeToken(TokenType::LBracket)) {
            lexWhitespaceToken();
            stmt->is_array = true;
            auto value = parseAccessChainOrValue(allocator);
            if(value) {
                stmt->array_value = value;
            }
            lexWhitespaceToken();
            if(!consumeToken(TokenType::RBracket)) {
                error("expected a ']' after the access chain value");
                return stmt;
            }
            if(!lexWhitespaceToken()) {
                error("expected whitespace after ']' in destruct statement");
                return stmt;
            }
        }
        auto value = parseAccessChainOrValue(allocator);
        if(value) {
            stmt->identifier = value;
        } else {
            error("expected a pointer value for the destruct statement");
            return stmt;
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
    unsigned int index = 0;
    do {
        lexWhitespaceAndNewLines();
        if(lexImplicitParams) {
            auto ampersand = consumeOfType(TokenType::AmpersandSym);
            if (ampersand) {
                auto is_mutable = consumeWSOfType(TokenType::MutKw) != nullptr; // optional mut keyword
                auto id = consumeIdentifierOrKeyword();
                if (id) {
                    const auto ref_to_linked  = new (allocator.allocate<ReferenceType>()) ReferenceType(new (allocator.allocate<LinkedType>()) LinkedType(id->value.str(), nullptr, loc_single(id)), loc_single(id), is_mutable);
                    auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(id->value.str(), ref_to_linked,
                                                                                        index, nullptr, true,
                                                                                        current_func_type, loc(ampersand, id));
                    parameters.emplace_back(param);
                    readWhitespace();
                    index++;
                    continue;
                } else {
                    error("expected a identifier right after '&' in the first function parameter as a 'self' parameter");
                    return false;
                }
            }
        }
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            readWhitespace();
            if(consumeToken(TokenType::ColonSym)) {
                lexWhitespaceToken();
                auto type = parseType(allocator);
                if(type) {
                    if(variadicParam && consumeToken(TokenType::TripleDotSym)) {
                        auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(id->value.str(), type, index, nullptr, false, current_func_type, loc_single(id));
                        parameters.emplace_back(param);
                        return true;
                    }
                    Value* defValue = nullptr;
                    if(defValues) {
                        readWhitespace();
                        if (consumeToken(TokenType::EqualSym)) {
                            readWhitespace();
                            auto expr = parseExpression(allocator);
                            if(expr) {
                                defValue = expr;
                            } else {
                                error("expected value after '=' for default value for the parameter");
                                break;
                            }
                        }
                    }
                    auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(id->value.str(), type, index, defValue, false, current_func_type, loc_single(id));
                    parameters.emplace_back(param);
                } else {
                    error("missing a type token for the function parameter, expected type after the colon");
                    return false;
                }
            } else {
                if(optionalTypes) {
                    auto param = new (allocator.allocate<FunctionParam>()) FunctionParam(id->value.str(), nullptr, index, nullptr, false, current_func_type, loc_single(id));
                    parameters.emplace_back(param);
                } else {
                    error("expected colon ':' in function parameter list after the parameter name ");
                    return false;
                }
            }
        }
        lexWhitespaceToken();
        index++;
    } while(consumeToken(TokenType::CommaSym));
    return false;
}

bool Parser::parseGenericParametersList(ASTAllocator& allocator, std::vector<GenericTypeParameter*>& params) {
    if(consumeToken(TokenType::LessThanSym)) {
        unsigned int param_index = 0;
        while(true) {
            lexWhitespaceToken();
            auto id = consumeIdentifierOrKeyword();
            if(!id) {
                break;
            }
            auto parameter = new (allocator.allocate<GenericTypeParameter>()) GenericTypeParameter(id->value.str(), nullptr, nullptr, parent_node, param_index, loc_single(id));
            params.emplace_back(parameter);
            lexWhitespaceToken();
            if(consumeToken(TokenType::ColonSym)) {
                lexWhitespaceToken();
                auto type = parseType(allocator);
                if(type) {
                    parameter->at_least_type = type;
                } else {
                    error("expected a type after ':' in generic parameter list");
                    return true;
                }
                lexWhitespaceToken();
            }
            if(consumeToken(TokenType::EqualSym)) {
                lexWhitespaceToken();
                auto type = parseType(allocator);
                if(type) {
                    parameter->def_type = type;
                } else {
                    error("expected a default type after '=' in generic parameter list");
                    return true;
                }
                lexWhitespaceToken();
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

FunctionDeclaration* Parser::parseFunctionStructureTokens(ASTAllocator& allocator, AccessSpecifier specifier, bool allow_declaration, bool allow_extensions) {

    if(!consumeWSOfType(TokenType::FuncKw)) {
        return nullptr;
    }

    std::vector<GenericTypeParameter*> gen_params;

    readWhitespace();

    if(parseGenericParametersList(allocator, gen_params) && has_errors) {
        return nullptr;
    }

    lexWhitespaceToken();

    FunctionDeclaration* decl;

    if(allow_extensions && consumeToken(TokenType::LParen)) {
        auto ext_func = new (allocator.allocate<ExtensionFunction>()) ExtensionFunction(loc_id("", {0, 0}), { "", nullptr, nullptr, 0 }, {}, nullptr, false, parent_node, 0, std::nullopt, specifier);;
        decl = ext_func;
        lexWhitespaceToken();
        auto id = consumeIdentifierOrKeyword();
        if(id) {
            ext_func->receiver.name = id->value.str();
        } else {
            error("expected identifier for receiver in extension function after '('");
            return decl;
        }
        lexWhitespaceToken();
        if(!consumeToken(TokenType::ColonSym)) {
            error("expected ':' in extension function after identifier for receiver");
            return decl;
        }
        lexWhitespaceToken();
        auto type = parseType(allocator);
        if(type) {
            ext_func->receiver.type = type;
        } else {
            error("expected type after ':' in extension function for receiver");
            return decl;
        }
        lexWhitespaceToken();
        if(!consumeToken(TokenType::RParen)) {
            error("expected ')' in extension function after receiver");
            return decl;
        }
        lexWhitespaceToken();
    } else {
        decl = new (allocator.allocate<FunctionDeclaration>()) FunctionDeclaration(loc_id("", {0, 0}), {}, nullptr, false, parent_node, 0, std::nullopt, specifier, false);
    }

    for(auto param : gen_params) {
        param->parent_node = decl;
    }
    decl->generic_params = std::move(gen_params);

    annotate(decl);

    auto name = consumeIdentifierOrKeyword();
    if(name) {
        decl->set_identifier(loc_id(name));
    } else {
        error("function name is missing after the keyword 'func'");
        return decl;
    }

    lexWhitespaceToken();

    if(!consumeToken(TokenType::LParen)) {
        error("expected a starting parenthesis ( in a function signature");
        return decl;
    }

    // inside the block allow return statements
    auto prev_func_type = current_func_type;
    current_func_type = decl;
    auto prev_parent_node = parent_node;
    parent_node = decl;

    auto isVariadic = parseParameterList(allocator, decl->params);
    decl->setIsVariadic(isVariadic);

    lexWhitespaceAndNewLines();

    if(!consumeToken(TokenType::RParen)) {
        error("expected a closing parenthesis ) when ending a function signature");
        return decl;
    }

    lexWhitespaceToken();

    auto& tok = *token;
    if(tok.type == TokenType::ColonSym) {
        token++;
        lexWhitespaceToken();
        auto type = parseType(allocator);
        if(type) {
            decl->returnType = type;
        } else {
            error("expected a return type for function after ':'");
            return decl;
        }
    } else {
        decl->returnType = new (allocator.allocate<VoidType>()) VoidType(loc_single(tok));
    }

    auto block = parseBraceBlock("function", decl, allocator);
    if(block.has_value()) {
        decl->body.emplace(block->parent_node, block->location);
        decl->body->nodes = std::move(block->nodes);
    } else if(!allow_declaration) {
        error("expected the function definition after the signature");
    }
    current_func_type = prev_func_type;
    parent_node = prev_parent_node;

    return decl;

}