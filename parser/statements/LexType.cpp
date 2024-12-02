// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "parser/Parser.h"
#include "ast/types/LinkedType.h"
#include "compiler/PrimitiveTypeMap.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/AccessChain.h"
#include "ast/types/LinkedValueType.h"
#include "ast/types/GenericType.h"
#include "ast/types/PointerType.h"
#include "ast/types/ReferenceType.h"
#include "ast/types/DynamicType.h"
#include "ast/types/ArrayType.h"
#include "ast/types/FunctionType.h"

BaseType* Parser::parseLambdaType(ASTAllocator& allocator, bool isCapturing) {
    auto t1 = consumeOfType(TokenType::LParen);
    if(t1) {

        std::vector<FunctionParam*> params;
        BaseType* returnType;
        bool isVariadic = parseParameterList(allocator, params);
        if(!consumeToken(TokenType::RParen)) {
            error("expected a ')' after the ')' in lambda function type");
        }
        lexWhitespaceToken();
        if(consumeToken(TokenType::LambdaSym)) {
            lexWhitespaceToken();
            auto type = parseType(allocator);
            if(type) {
                returnType = type;
            } else {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        return new (allocator.allocate<FunctionType>()) FunctionType(params, returnType, isVariadic, isCapturing, parent_node, loc_single(t1));
    } else {
        return nullptr;
    }
}

bool Parser::lexLambdaTypeTokens(unsigned int start) {
    if(lexOperatorToken(TokenType::LParen)) {
        lexParameterList();
        if(!lexOperatorToken(TokenType::RParen)) {
            error("expected a ')' after the ')' in lambda function type");
        }
        lexWhitespaceToken();
        if(lexOperatorToken(TokenType::LambdaSym)) {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                error("expected a return type for lambda function type");
            }
        } else {
            error("expected '=>' for lambda function type");
        }
        compound_from(start, LexTokenType::CompFunctionType);
        return true;
    } else {
        return false;
    }
}

bool Parser::lexGenericTypeAfterId(unsigned int start) {
    if(lexOperatorToken(TokenType::LessThanSym)) {
        do {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while(lexOperatorToken(TokenType::CommaSym));
        lexWhitespaceToken();
        if(!lexOperatorToken(TokenType::GreaterThanSym)) {
            error("expected '>' for generic type");
        }
        compound_from(start, LexTokenType::CompGenericType);
        return true;
    } else {
        return false;
    }
}

BaseType* Parser::parseGenericTypeAfterId(ASTAllocator& allocator, BaseType* idType) {
    if(consumeToken(TokenType::LessThanSym)) {
        std::vector<BaseType*> types;
        do {
            lexWhitespaceToken();
            auto type = parseType(allocator);
            if(type) {
                types.emplace_back(type);
            } else {
                break;
            }
            lexWhitespaceToken();
        } while(consumeToken(TokenType::CommaSym));
        lexWhitespaceToken();
        if(!consumeToken(TokenType::GreaterThanSym)) {
            error("expected '>' for generic type");
        }
        // TODO Generic Type doesn't support linked value type
        return new (allocator.allocate<GenericType>()) GenericType((LinkedType*) idType, std::move(types));
    } else {
        return idType;
    }
}

bool Parser::lexRefOrGenericType() {
    unsigned start = tokens_size();
    auto id = consumeIdentifierOrKeyword();
    if(!id) {
        error("missing struct / interface name in inheritance list of the struct");
        return false;
    }
    emplace(LexTokenType::Type, id->position, std::string(id->value));
    lexWhitespaceToken();
    lexGenericTypeAfterId(start);
    return true;
}

BaseType* Parser::parseArrayAndPointerTypesAfterTypeId(ASTAllocator& allocator, BaseType* typeId) {
    auto t1 = consumeOfType(TokenType::LBracket);
    if(t1) {
        // optional array size
        auto expr = parseExpression(allocator);
        auto t2 = consumeOfType(TokenType::RBracket);
        if(!t2) {
            error("expected ']' for array type");
            return typeId;
        }
        typeId = new (allocator.allocate<ArrayType>()) ArrayType(typeId, expr, loc(t1, t2));
    }
    while(true) {
        auto t = consumeOfType(TokenType::MultiplySym);
        if(t) {
            warning("deprecated syntax, pointer should be before type");
            typeId = new (allocator.allocate<PointerType>()) PointerType(typeId, loc_single(t));
        } else {
            break;
        }
    }
    auto t = consumeOfType(TokenType::AmpersandSym);
    if(t) {
        warning("deprecated syntax, reference should be before type");
        typeId = new (allocator.allocate<ReferenceType>()) ReferenceType(typeId, loc_single(t));
    }
    return typeId;
}

void Parser::lexArrayAndPointerTypesAfterTypeId(unsigned int start) {
    if(lexOperatorToken(TokenType::LBracket)) {
        // optional array size
        lexExpressionTokens();
        if(!lexOperatorToken(TokenType::RBracket)) {
            error("expected ']' for array type");
            return;
        }
        compound_from(start, LexTokenType::CompArrayType);
    }
    while(lexOperatorToken(TokenType::MultiplySym)) {
        warning("deprecated syntax, pointer should be before type");
        compound_from(start, LexTokenType::CompPointerType);
    }
    if(lexOperatorToken(TokenType::AmpersandSym)) {
        warning("deprecated syntax, reference should be before type");
        compound_from(start, LexTokenType::CompReferenceType);
    }
}

BaseType* Parser::parseTypeId(ASTAllocator& allocator, Token* type) {
    Token* first_type = type;
    AccessChain* chain = nullptr;
    while(true) {
        if(token->type == TokenType::DoubleColonSym) {
            token++;
            auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(type->value), loc_single(type), true);
            auto new_type = consumeIdentifierOrKeyword();
            if(!new_type) {
                error("expected an identifier after '" + std::string(type->value) + "::' for a type");
                return nullptr;
            } else {
                if(chain) {
                    chain->values.emplace_back(id);
                } else {
                    chain = new (allocator.allocate<AccessChain>()) AccessChain({ id }, parent_node, false, 0);
                }
                type = new_type;
            }
        } else {
            if(chain) {
                auto id = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(type->value), loc_single(type));
                chain->values.emplace_back(id);
                chain->location = loc(first_type, type);
                return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain, chain->location);
            } else {
                auto primitive = TypeMakers::PrimitiveMap.find(type->value);
                if (primitive == TypeMakers::PrimitiveMap.end()) {
                    return new (allocator.allocate<LinkedType>()) LinkedType(std::string(type->value), loc_single(type));
                } else {
                    return primitive->second(allocator, is64Bit, loc_single(type));
                }
            }
        }
    }
}

BaseType* Parser::parseType(ASTAllocator& allocator) {

    auto t1 = consumeOfType(TokenType::LBracket);
    if(t1) {
        if(!consumeToken(TokenType::RBracket)) {
            error("expected ']' after '[' for lambda type");
            return nullptr;
        }
        lexWhitespaceToken();
        auto lambdaType = parseLambdaType(allocator, true);
        if(lambdaType) {
            return lambdaType;
        } else {
            error("expected a lambda type after '[]'");
            return nullptr;
        }
    }

    auto lambdaType = parseLambdaType(allocator, false);
    if(lambdaType) {
        return lambdaType;
    }

    auto ptrToken = consumeOfType(TokenType::MultiplySym);
    if(ptrToken) {
        auto type = parseType(allocator);
        if(type) {
            return new (allocator.allocate<PointerType>()) PointerType(type, loc_single(ptrToken));
        } else {
            error("expected a type after the *");
            return nullptr;
        }
    } else {
        auto refToken = consumeOfType(TokenType::AmpersandSym);
        if(refToken) {
            auto type = parseType(allocator);
            if(type) {
                return new (allocator.allocate<ReferenceType>()) ReferenceType(type, loc_single(refToken));
            } else {
                error("expected a type after the &");
                return nullptr;
            }
        }
    }

    auto dynToken = consumeWSOfType(TokenType::DynKw);
    if(dynToken) {
        auto type = parseType(allocator);
        if(type) {
            return new (allocator.allocate<DynamicType>()) DynamicType(type, loc_single(dynToken));
        } else {
            error("expected a type after the qualifier");
            return nullptr;
        }
    } else {
        auto mutToken = consumeWSOfType(TokenType::MutKw);
        if(mutToken) {
            auto type = parseType(allocator);
            if(type) {
                type->make_mutable(type->kind());
                return type;
            } else {
                error("expected a type after the qualifier");
                return nullptr;
            }
        }
    }
    auto typeToken = consumeIdentifierOrKeyword();
    if(!typeToken) return nullptr;
    unsigned start = tokens_size();
    auto type = parseTypeId(allocator, typeToken);
    if(!type) {
        return nullptr;
    }
    type = parseGenericTypeAfterId(allocator, type);
    type = parseArrayAndPointerTypesAfterTypeId(allocator, type);
    return type;
}

bool Parser::lexTypeTokens() {

    if(lexOperatorToken(TokenType::LBracket)) {
        unsigned start = tokens_size() - 1;
        if(!lexOperatorToken(TokenType::RBracket)) {
            error("expected ']' after '[' for lambda type");
            return true;
        }
        lexWhitespaceToken();
        if(!lexLambdaTypeTokens(start)) {
            error("expected a lambda type after '[]'");
        }
        return true;
    }

    if(lexLambdaTypeTokens(tokens_size())) {
        return true;
    }

    if(lexOperatorToken(TokenType::MultiplySym)) {
        unsigned start = tokens_size() - 1;
        if(!lexTypeTokens()) {
            error("expected a type after the *");
            return false;
        }
        compound_from(start, LexTokenType::CompPointerType);
        return true;
    } else if(lexOperatorToken(TokenType::AmpersandSym)) {
        unsigned start = tokens_size() - 1;
        if(!lexTypeTokens()) {
            error("expected a type after the &");
            return false;
        }
        compound_from(start, LexTokenType::CompReferenceType);
        return true;
    }

    if(lexWSKeywordToken(TokenType::DynKw) || lexWSKeywordToken(TokenType::MutKw)) {
        unsigned start = tokens_size() - 1;
        if(!lexTypeTokens()) {
            error("expected a type after the qualifier");
            return false;
        }
        compound_from(start, LexTokenType::CompQualifiedType);
        return true;
    }

    auto type = consumeIdentifierOrKeyword();
    if(!type) return false;
    unsigned start = tokens_size();
    if(!lexTypeId(type)) {
        return true;
    }
    lexGenericTypeAfterId(start);
    lexArrayAndPointerTypesAfterTypeId(start);

    return true;

}