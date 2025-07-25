// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/base/TypeBuilder.h"
#include "ast/values/BoolValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/VariableIdentifier.h"
#include "ast/values/FunctionCall.h"
#include "ast/values/IndexOperator.h"
#include "ast/types/LinkedType.h"
#include "ast/types/GenericType.h"
#include "ast/values/StructValue.h"
#include "ast/types/LinkedValueType.h"
#include "ast/values/AccessChain.h"
#include "ast/values/AddrOfValue.h"
#include "ast/values/DereferenceValue.h"
#include "ast/values/ComptimeValue.h"
#include "ast/values/SizeOfValue.h"
#include "ast/values/AlignOfValue.h"

Value* Parser::parseAccessChainOrKwValue(ASTAllocator& allocator, bool parseStruct) {
    switch(token->type) {
        case TokenType::NullKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), loc_single(t));
        }
        case TokenType::TrueKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(true, typeBuilder.getBoolType(), loc_single(t));
        }
        case TokenType::FalseKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), loc_single(t));
        }
        default:
            return parseAccessChain(allocator, parseStruct);
    }
}

Value* Parser::parseAccessChain(ASTAllocator& allocator, bool parseStruct) {

    auto id = consumeIdentifierOrKeyword();
    if(id == nullptr) {
        return nullptr;
    }

    if(id->type == TokenType::StructKw) {
        const auto value = (Value*) parseStructValue(allocator, nullptr, id->position);
        if(!value) {
            unexpected_error("expected '{' after the struct keyword for unnamed struct value");
        }
#ifdef LSP_BUILD
        id->linked = value;
#endif
        return value;
    }

    auto tokenType = token->type;

    switch(tokenType) {
        case TokenType::NewLine:
        case TokenType::RParen:
        case TokenType::RBrace:
        case TokenType::RBracket:
        case TokenType::CommaSym:
            return new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));;
        case TokenType::LBrace: {
            auto ref_type = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, id->value));
            return parseStructValue(allocator, ref_type, id->position);
        }
        default:
            break;
    }

    auto chain = new (allocator.allocate<AccessChain>()) AccessChain(false, loc_single(id));
    auto identifier = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));
    chain->values.emplace_back(identifier);

#ifdef LSP_BUILD
    id->linked = identifier;
#endif

    return parseAccessChainAfterId(allocator, chain, id->position, parseStruct);

}

AddrOfValue* Parser::parseAddrOfValue(ASTAllocator& allocator) {
    auto token1 = consumeOfType(TokenType::AmpersandSym);
    if (token1) {
        const auto is_mutable = consumeToken(TokenType::MutKw);
        auto chain = parseAccessChainOrAddrOf(allocator, true);
        if (chain) {
            return new(allocator.allocate<AddrOfValue>()) AddrOfValue(chain, is_mutable, loc_single(token1));
        } else {
            unexpected_error("expected a value after '&' for address of");
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

DereferenceValue* Parser::parseDereferenceValue(ASTAllocator& allocator) {
    auto token2 = consumeOfType(TokenType::MultiplySym);
    if (token2) {
        auto chain = parseAccessChainOrAddrOf(allocator, false);
        if (chain) {
            return new(allocator.allocate<DereferenceValue>()) DereferenceValue(chain, loc_single(token2));
        } else {
            const auto expr = parseParenExpression(allocator);
            if(expr) {
                return new(allocator.allocate<DereferenceValue>()) DereferenceValue(expr, loc_single(token2));
            } else {
                unexpected_error("expected a value after '*' for dereference");
                return nullptr;
            }
        }
    } else {
        return nullptr;
    }
}

Value* Parser::parseAccessChainOrAddrOf(ASTAllocator& allocator, bool parseStruct) {
    switch (token->type) {
        case TokenType::AmpersandSym:
            return (Value*) parseAddrOfValue(allocator);
        case TokenType::MultiplySym:
            return (Value*) parseDereferenceValue(allocator);
        case TokenType::DoublePlusSym:
            return parsePreIncDecValue(allocator, true);
        case TokenType::DoubleMinusSym:
            return parsePreIncDecValue(allocator, false);
        case TokenType::NullKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<NullValue>()) NullValue(typeBuilder.getNullPtrType(), loc_single(t));
        }
        case TokenType::TrueKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(true, typeBuilder.getBoolType(), loc_single(t));
        }
        case TokenType::FalseKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(false, typeBuilder.getBoolType(), loc_single(t));
        }
        case TokenType::UnsafeKw: {
            token++;
            return parseUnsafeValue(allocator);
        }
        case TokenType::ComptimeKw: {
            token++;
            return parseComptimeValue(allocator);
        }
        case TokenType::SizeOfKw: {
            token++;
            return parseSizeOfValue(allocator);
        }
        case TokenType::AlignOfKw: {
            token++;
            return parseAlignOfValue(allocator);
        }
        default:
            return (Value*) parseAccessChain(allocator, parseStruct);
    }
}

Value* Parser::parseAccessChainRecursive(ASTAllocator& allocator, AccessChain* chain, Position& start, bool parseStruct) {
    auto id = parseVariableIdentifier(allocator);
    if(id) {
        chain->values.emplace_back(id);
    } else {
        return nullptr;
    }
    return parseAccessChainAfterId(allocator, chain, start, parseStruct);
}

ChainValue* take_parent(ASTAllocator& allocator, AccessChain* chain, SourceLocation location) {
    if(chain->values.size() == 1) {
        const auto parent_val = chain->values.back();
        chain->values.pop_back();
        return parent_val;
    } else {
        return new (allocator.allocate<AccessChain>()) AccessChain(std::move(chain->values), false, location);
    }
}

FunctionCall* Parser::parseFunctionCall(ASTAllocator& allocator, AccessChain* chain) {
    auto& lParenTok = *token;
    if(lParenTok.type == TokenType::LParen) {
        const auto location = loc_single(lParenTok);
        auto call = new (allocator.allocate<FunctionCall>()) FunctionCall(take_parent(allocator, chain, location), location);
        token++;
        do {
            consumeNewLines();
            auto expr = parseExpressionOrArrayOrStruct(allocator);
            if(expr) {
                call->values.emplace_back(expr);
            } else {
                break;
            }
        } while (consumeToken(TokenType::CommaSym));
        consumeNewLines();
        if (!consumeToken(TokenType::RParen)) {
            unexpected_error("expected a ')' for a function call, after starting '('");
            return call;
        }
        return call;
    } else {
        return nullptr;
    }
}

void Parser::parseGenericArgsList(std::vector<TypeLoc>& outArgs, ASTAllocator& allocator) {
    if(consumeToken(TokenType::LessThanSym)) {
        do {
            auto type = parseTypeLoc(allocator);
            if (type) {
                outArgs.emplace_back(type);
            } else {
                break;
            }
        } while(consumeToken(TokenType::CommaSym));
        if (!consumeToken(TokenType::GreaterThanSym)) {
            unexpected_error("expected a '>' for generic list in function call");
        }
    }
}

BaseType* Parser::ref_type_from(ASTAllocator& allocator, AccessChain* chain) {
    if(chain->values.size() == 1) {
        auto val = (VariableIdentifier*) chain->values.back();
        return new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, val->value));
    } else {
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain);
    }
}

Value* Parser::parseAccessChainAfterId(ASTAllocator& allocator, AccessChain* chain, Position& start, bool parseStruct) {

    if(parseStruct) {
        if(token->type == TokenType::LBrace) {
            return parseStructValue(allocator, ref_type_from(allocator, chain), start);
        }
    }

    // when there is generic args after the identifier StructName<int, float> or func_name<int, float>()
    if (token->type == TokenType::LessThanSym && isGenericEndAhead()) {
        std::vector<TypeLoc> genArgs;
        parseGenericArgsList(genArgs, allocator);
        if(token->type == TokenType::LParen) {
            auto call = parseFunctionCall(allocator, chain);
            call->generic_list = std::move(genArgs);
            chain->values.emplace_back(call);
        } else if(parseStruct && token->type == TokenType::LBrace) {
            if(chain->values.size() == 1) {
                auto id = (VariableIdentifier*) chain->values.back();
                auto ref_type = new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, id->value));
                auto gen_type = new (allocator.allocate<GenericType>()) GenericType(ref_type, std::move(genArgs));
                return parseStructValue(allocator, gen_type, start);
            } else {
                // TODO this shouldn't be the case, Generic Type currently supports only LinkedType, it should support LinkedValueType
                error("multiple values in access chain of struct value are not supported");
                return nullptr;
            }
        } else {
            unexpected_error("expected a '(' or '{' after the generic list for a function call or struct initialization");
        }
    }

    // index operator, function call with generic items
    while(token->type == TokenType::LParen || token->type == TokenType::LBracket) {
        if(token->type == TokenType::LBracket) {
            const auto location = loc_single(token);
            auto indexOp = new (allocator.allocate<IndexOperator>()) IndexOperator(take_parent(allocator, chain, location), location);
            chain->values.emplace_back(indexOp);
            while (token->type == TokenType::LBracket) {
                token++;
                auto expr = parseExpression(allocator);
                if (!expr) {
                    error("expected an expression in indexing operators for access chain");
                    return chain;
                }
                indexOp->values.emplace_back(expr);
                auto rbToken = consumeOfType(TokenType::RBracket);
                if (!rbToken) {
                    error("expected a closing bracket ] in access chain");
                    return chain;
                }
            }
        }
        while(true) {
            if (token->type == TokenType::LParen) {
                auto call = parseFunctionCall(allocator, chain);
                chain->values.emplace_back(call);
            } else if(token->type == TokenType::LessThanSym) {
                std::vector<TypeLoc> genArgs;
                parseGenericArgsList(genArgs, allocator);
                if(token->type == TokenType::LParen){
                    auto call = parseFunctionCall(allocator, chain);
                    call->generic_list = std::move(genArgs);
                    chain->values.emplace_back(call);
                } else {
                    error("expected a '(' after the generic list in function call");
                }
            } else {
                break;
            }
        }
    }

    // TODO false for parseStruct being sent here, should be changed to parseStruct
    if(consumeToken(TokenType::DotSym)) {
        return parseAccessChainRecursive(allocator, chain, start, false);
    } else if(consumeToken(TokenType::DoubleColonSym)) {
        auto lastId = chain->values.back();
        auto id = lastId->as_identifier();
        if(id) {
            id->is_ns = true;
        } else {
            error("double colon '::' after unknown value");
        }
        return parseAccessChainRecursive(allocator, chain, start, parseStruct);
    }

    return chain;

}