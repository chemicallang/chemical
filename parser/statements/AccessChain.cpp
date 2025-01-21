// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
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

bool Parser::consumeToken(enum TokenType type) {
    if(token->type == type) {
        token++;
        return true;
    } else {
        return false;
    }
}

Token* Parser::consumeOfType(TokenType type) {
    auto& t = *token;
    if(t.type == type) {
        token++;
        return &t;
    } else {
        return nullptr;
    }
}

Token* Parser::consumeWSOfType(enum TokenType type) {
    auto& t = *token;
    if(t.type == type) {
        token++;
        return &t;
    } else {
        return nullptr;
    }
}

Value* Parser::parseAccessChainOrKwValue(ASTAllocator& allocator, bool parseStruct) {
    switch(token->type) {
        case TokenType::NullKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<NullValue>()) NullValue(loc_single(t));
        }
        case TokenType::TrueKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(true, loc_single(t));
        }
        case TokenType::FalseKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(false, loc_single(t));
        }
        default:
            return parseAccessChain(allocator, parseStruct);
    }
}

Value* Parser::parseSizeOfValue(ASTAllocator& allocator) {
    const auto tok = token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        token++;
    } else {
        error("expected '{' or '(' when parsing sizeof");
        return nullptr;
    }
    auto type = parseType(allocator);
    if(type) {
        auto last = token;
        auto value = new (allocator.allocate<SizeOfValue>()) SizeOfValue(type, loc(tok, last));
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            token++;
        } else {
            error("expected '}' or '}' after the type when parsing sizeof");
        }
        return value;
    } else {
        error("expected a type in #sizeof");
        return nullptr;
    }
}

Value* Parser::parseAlignOfValue(ASTAllocator& allocator) {
    auto tok = token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        token++;
    } else {
        error("expected '{' or '(' when parsing alignof");
        return nullptr;
    }
    auto type = parseType(allocator);
    if(type) {
        auto last = token;
        auto value = new (allocator.allocate<AlignOfValue>()) AlignOfValue(type, loc(tok, last));
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            token++;
        } else {
            error("expected '}' or '}' after the type when parsing sizeof");
        }
        return value;
    } else {
        error("expected a type in #alignof");
        return nullptr;
    }
}

Value* parseSizeOfValue(Parser* parser, ASTAllocator* allocator_ptr) {
    return parser->parseSizeOfValue(*allocator_ptr);
}

Value* parseAlignOfValue(Parser* parser, ASTAllocator* allocator_ptr) {
    return parser->parseAlignOfValue(*allocator_ptr);
}

Value* Parser::parseAccessChain(ASTAllocator& allocator, bool parseStruct) {

    auto id = consumeIdentifierOrKeyword();
    if(id == nullptr) {
        return nullptr;
    }

    if(id->type == TokenType::StructKw) {
        const auto value = (Value*) parseStructValue(allocator, nullptr, id->position);
        if(!value) {
            error("expected '{' after the struct keyword for unnamed struct value");
        }
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
            auto ref_type = new (allocator.allocate<LinkedType>()) LinkedType(allocate_view(allocator, id->value), loc_single(id));
            return parseStructValue(allocator, ref_type, id->position);
        }
        default:
            break;
    }

    auto chain = new (allocator.allocate<AccessChain>()) AccessChain({}, false, loc_single(id));
    auto identifier = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(allocate_view(allocator, id->value), loc_single(id));
    chain->values.emplace_back(identifier);

    return parseAccessChainAfterId(allocator, chain, id->position, parseStruct);

}

AddrOfValue* Parser::parseAddrOfValue(ASTAllocator& allocator) {
    auto token1 = consumeOfType(TokenType::AmpersandSym);
    if (token1) {
        auto chain = parseAccessChainOrAddrOf(allocator, true);
        if (chain) {
            return new(allocator.allocate<AddrOfValue>()) AddrOfValue(chain, loc_single(token1));
        } else {
            error("expected a value after '&' for address of");
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
                error("expected a value after '*' for dereference");
                return nullptr;
            }
        }
    } else {
        return nullptr;
    }
}

Value* parseComptimeValue(Parser* parser, ASTAllocator* allocator_ptr) {
    auto tok = parser->token;
    const auto first_type = tok->type;
    if(first_type == TokenType::LBrace || first_type == TokenType::LParen) {
        parser->token++;
    } else {
        parser->error("expected '{' or '(' when parsing comptime value");
        return nullptr;
    }
    auto& allocator = *allocator_ptr;
    auto expr = parser->parseExpression(allocator);
    if(expr) {
        auto last = parser->token;
        auto evaluated = new (allocator.allocate<ComptimeValue>()) ComptimeValue(allocator_ptr, expr);
        const auto last_type = last->type;
        if((first_type == TokenType::LBrace && last_type == TokenType::RBrace) || (first_type == TokenType::LParen && last_type == TokenType::RParen)) {
            parser->token++;
        } else {
            parser->error("expected '}' or '}' after the type when parsing comptime value");
        }
        return evaluated;
    } else {
        parser->error("expected a value in #eval");
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
            return new(allocator.allocate<NullValue>()) NullValue(loc_single(t));
        }
        case TokenType::TrueKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(true, loc_single(t));
        }
        case TokenType::FalseKw: {
            const auto t = token;
            token++;
            return new(allocator.allocate<BoolValue>()) BoolValue(false, loc_single(t));
        }
        case TokenType::ComptimeKw: {
            token++;
            return parseComptimeValue(this, &allocator);
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
        auto call = new (allocator.allocate<FunctionCall>()) FunctionCall(take_parent(allocator, chain, location), {}, location);
        token++;
        do {
            consumeNewLines();
            auto expr = parseExpression(allocator, true);
            if(expr) {
                call->values.emplace_back(expr);
            } else {
                auto init = parseArrayInit(allocator);
                if(init) {
                    call->values.emplace_back(init);
                }
            }
        } while (consumeToken(TokenType::CommaSym));
        consumeNewLines();
        if (!consumeToken(TokenType::RParen)) {
            error("expected a ')' for a function call, after starting '('");
            return call;
        }
        return call;
    } else {
        return nullptr;
    }
}

void Parser::parseGenericArgsList(std::vector<BaseType*>& outArgs, ASTAllocator& allocator) {
    if(consumeToken(TokenType::LessThanSym)) {
        do {
            auto type = parseType(allocator);
            if (type) {
                outArgs.emplace_back(type);
            } else {
                break;
            }
        } while(consumeToken(TokenType::CommaSym));
        if (!consumeToken(TokenType::GreaterThanSym)) {
            error("expected a '>' for generic list in function call");
        }
    }
}

BaseType* Parser::ref_type_from(ASTAllocator& allocator, AccessChain* chain) {
    if(chain->values.size() == 1) {
        auto val = (VariableIdentifier*) chain->values.back();
        return new (allocator.allocate<LinkedType>()) LinkedType(allocate_view(allocator, val->value), val->location);
    } else {
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain, chain->location);
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
        std::vector<BaseType*> genArgs;
        parseGenericArgsList(genArgs, allocator);
        if(token->type == TokenType::LParen) {
            auto call = parseFunctionCall(allocator, chain);
            call->generic_list = std::move(genArgs);
            chain->values.emplace_back(call);
        } else if(parseStruct && token->type == TokenType::LBrace) {
            if(chain->values.size() == 1) {
                auto id = (VariableIdentifier*) chain->values.back();
                auto ref_type = new (allocator.allocate<LinkedType>()) LinkedType(allocate_view(allocator, id->value), id->location);
                auto gen_type = new (allocator.allocate<GenericType>()) GenericType(ref_type, std::move(genArgs));
                return parseStructValue(allocator, gen_type, start);
            } else {
                // TODO this shouldn't be the case, Generic Type currently supports only LinkedType, it should support LinkedValueType
                error("multiple values in access chain of struct value are not supported");
                return nullptr;
            }
        } else {
            error("expected a '(' or '{' after the generic list for a function call or struct initialization");
        }
    }

    // index operator, function call with generic items
    while(token->type == TokenType::LParen || token->type == TokenType::LBracket) {
        if(token->type == TokenType::LBracket) {
            const auto location = loc_single(token);
            auto indexOp = new (allocator.allocate<IndexOperator>()) IndexOperator(take_parent(allocator, chain, location), {}, location);
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
                std::vector<BaseType*> genArgs;
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