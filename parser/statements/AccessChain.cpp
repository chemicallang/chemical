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
#include "ast/values/SizeOfValue.h"

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

void Parser::parseAccessChain(ASTAllocator& allocator, std::vector<ChainValue*>& values) {

    auto id = consumeIdentifierOrKeyword();
    if(id == nullptr) {
        return;
    }

    auto identifier = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));
    values.emplace_back(identifier);

#ifdef LSP_BUILD
    id->linked = identifier;
#endif

    // ignore the returned struct value
    parseAccessChainAfterId(allocator, values, id->position);

}

Value* Parser::singlify_chain(AccessChain* chain) {
    if(chain->values.size() == 1) {
        return chain->values.back();
    } else {
        return chain;
    }
}

Value* Parser::parseAccessChain(ASTAllocator& allocator, bool parseStruct) {

    auto id = consumeIdentifierOrKeyword();
    if(id == nullptr) {
        return nullptr;
    }

    auto chain = new (allocator.allocate<AccessChain>()) AccessChain(loc_single(id));
    auto identifier = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));
    chain->values.emplace_back(identifier);

#ifdef LSP_BUILD
    id->linked = identifier;
#endif

    const auto structVal = parseAccessChainAfterId(allocator, chain->values, id->position, parseStruct);

    // TODO: use singlify chain here, this causes a bug in the c translation
    //  that bug must be fixed, so C output is same with and without access chain wrap
    return structVal ? structVal : chain;

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

Value* Parser::parseLhsValue(ASTAllocator& allocator) {
    switch (token->type) {
        case TokenType::AmpersandSym:
            return (Value*) parseAddrOfValue(allocator);
        case TokenType::MultiplySym:
            return (Value*) parseDereferenceValue(allocator);
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
        default:
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
        case TokenType::DynKw: {
            token++;
            return parseDynamicValue(allocator);
        }
        case TokenType::StructKw: {
            auto& t = *token;
            token++;
            const auto value = (Value*) parseStructValue(allocator, nullptr, t.position);
            if(!value) {
                unexpected_error("expected '{' after the struct keyword for unnamed struct value");
            }
#ifdef LSP_BUILD
            t.linked = value;
#endif
            return value;
        }
        default:
            return (Value*) parseAccessChain(allocator, parseStruct);
    }
}

Value* Parser::parseAccessChainRecursive(ASTAllocator& allocator, std::vector<ChainValue*>& values, Position& start, bool parseStruct) {
    auto id = parseVariableIdentifier(allocator);
    if(id) {
        values.emplace_back(id);
    } else {
        return nullptr;
    }
    return parseAccessChainAfterId(allocator, values, start, parseStruct);
}

ChainValue* take_parent(ASTAllocator& allocator, std::vector<ChainValue*>& values, SourceLocation location) {
    if(values.size() == 1) {
        const auto parent_val = values.back();
        values.pop_back();
        return parent_val;
    } else {
        return new (allocator.allocate<AccessChain>()) AccessChain(std::move(values), location);
    }
}

FunctionCall* Parser::parseFunctionCall(ASTAllocator& allocator, std::vector<ChainValue*>& values) {
    auto& lParenTok = *token;
    if(lParenTok.type == TokenType::LParen) {
        const auto location = loc_single(lParenTok);
        auto call = new (allocator.allocate<FunctionCall>()) FunctionCall(take_parent(allocator, values, location), location);
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
void Parser::parseGenericArgsListNoStart(std::vector<TypeLoc>& outArgs, ASTAllocator& allocator) {
    do {
        auto type = parseTypeLoc(allocator);
        if (type) {
            outArgs.emplace_back(type);
        } else {
            break;
        }
    } while(consumeToken(TokenType::CommaSym));
    if (!consumeGenericClose()) {
        unexpected_error("expected a '>' for generic list in function call");
    }
}

LinkedType* Parser::ref_type_from(ASTAllocator& allocator, std::vector<ChainValue*>& values) {
    if(values.size() == 1 && values.back()->kind() == ValueKind::Identifier) {
        auto val = values.back()->as_identifier_unsafe();
        return new (allocator.allocate<NamedLinkedType>()) NamedLinkedType(allocate_view(allocator, val->value));
    } else {
        const auto loc = values.front()->encoded_location();
        const auto chain = new (allocator.allocate<AccessChain>()) AccessChain(std::move(values), loc);
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain);
    }
}

Value* Parser::parseAccessChainAfterId(ASTAllocator& allocator, std::vector<ChainValue*>& values, Position& start, bool parseStruct, bool parseGenList) {

    // consume access chain values
    while(true) {

        switch(token->type) {
            case TokenType::DoubleColonSym: {
                // set previous identifier is_ns
                auto lastId = values.back();
                auto last_id = lastId->as_identifier();
                if (last_id) {
                    last_id->is_ns = true;
                } else {
                    error("double colon '::' after unknown value");
                }
                break;
            }
            case TokenType::DotSym:
                break;
            default:
                goto end_loop;

        }

        token++;

        auto next_id = consumeIdentifierOrKeyword();
        if(next_id) {
            values.emplace_back(
                    new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, next_id->value), loc_single(next_id))
            );
        } else {
            error("expected an identifier after '.' or '::'");
            break;
        }

    }

    end_loop:

    // generic list -> struct/call
    // struct
    // index op -> recursive chain
    // function call -> recursive chain
    switch(token->type) {
        case TokenType::LBrace:
            if(parseStruct) {
                return parseStructValue(allocator, ref_type_from(allocator, values), start);
            } else {
                error("unexpected l-brace, struct value not expected");
            }
            break;
        case TokenType::LBracket: {
            const auto location = loc_single(token);
            auto indexOp = new(allocator.allocate<IndexOperator>()) IndexOperator(take_parent(allocator, values, location), location);
            values.emplace_back(indexOp);
            token++;
            auto expr = parseExpression(allocator);
            if (!expr) {
                error("expected an expression in indexing operators for access chain");
                return nullptr;
            }
            indexOp->idx = expr;
            if (!consumeToken(TokenType::RBracket)) {
                error("expected a closing bracket ] in access chain");
                return nullptr;
            }
            // parse recursive access chain
            return parseAccessChainAfterId(allocator, values, start, false, false);
        }
        case TokenType::LParen: {
            auto call = parseFunctionCall(allocator, values);
            values.emplace_back(call);
            // parse access chain recursive
            return parseAccessChainAfterId(allocator, values, start, false, false);
        }
        case TokenType::TurboFishSym: {
            consumeAny(); // consume the turbo fish
            // generic list detected
            std::vector<TypeLoc> genArgs;
            parseGenericArgsListNoStart(genArgs, allocator);
            // generic list -> struct/call
            switch(token->type) {
                case TokenType::LBrace:
                    if(parseStruct) {
                        const auto ref_type = ref_type_from(allocator, values);
                        const auto gen_type = new (allocator.allocate<GenericType>()) GenericType(ref_type, std::move(genArgs));
                        return parseStructValue(allocator, gen_type, start);
                    } else {
                        error("unexpected l-brace, struct value not expected");
                    }
                    break;
                case TokenType::LParen: {
                    auto call = parseFunctionCall(allocator, values);
                    call->generic_list = std::move(genArgs);
                    values.emplace_back(call);
                    return parseAccessChainAfterId(allocator, values, start, false, false);
                }
                default:
                    break;
            }
        }
        case TokenType::LessThanSym:
            if(parseGenList && isGenericEndAhead()) {
                // generic list detected
                std::vector<TypeLoc> genArgs;
                warning("use the turbofish operator for faster compilation");
                consumeAny(); // consume the less than symbol
                parseGenericArgsListNoStart(genArgs, allocator);
                // generic list -> struct/call
                switch(token->type) {
                    case TokenType::LBrace:
                        if(parseStruct) {
                            const auto ref_type = ref_type_from(allocator, values);
                            const auto gen_type = new (allocator.allocate<GenericType>()) GenericType(ref_type, std::move(genArgs));
                            return parseStructValue(allocator, gen_type, start);
                        } else {
                            error("unexpected l-brace, struct value not expected");
                        }
                        break;
                    case TokenType::LParen: {
                        auto call = parseFunctionCall(allocator, values);
                        call->generic_list = std::move(genArgs);
                        values.emplace_back(call);
                        return parseAccessChainAfterId(allocator, values, start, false, false);
                    }
                    default:
                        break;
                }
            } else {
                // less than operator x::y.z < 123
                break;
            }
        default:
            break;
    }

    return nullptr;

}