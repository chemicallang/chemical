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

// function not required
// TODO return the string view and consume token
bool Parser::lexVariableToken() {
    auto id = consumeIdentifierOrKeyword();
    if(id) {
        emplace(LexTokenType::Variable, id->position, std::string(id->value));
        return true;
    } else {
        return false;
    }
}

// function not required
// TODO return the string view and consume token
bool Parser::lexIdentifierToken() {
    auto id = consumeIdentifierOrKeyword();
    if(id) {
        emplace(LexTokenType::Identifier, id->position, std::string(id->value));
        return true;
    } else {
        return false;
    }
}

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
        readWhitespace();
        return &t;
    } else {
        return nullptr;
    }
}

const std::unordered_map<std::string_view, ValueCreatorFn> ValueCreators = {
        {"null", [](Parser *parser, ASTAllocator& allocator, Token* token) -> Value* {
            return new (allocator.allocate<NullValue>()) NullValue(parser->loc_single(token));
        }},
        {"true", [](Parser *parser, ASTAllocator& allocator, Token* token) -> Value* {
            return new (allocator.allocate<BoolValue>()) BoolValue(true, parser->loc_single(token));
        }},
        {"false", [](Parser *parser, ASTAllocator& allocator, Token* token) -> Value* {
            return new (allocator.allocate<BoolValue>()) BoolValue(false, parser->loc_single(token));
        }}
};

Value* Parser::parseAccessChain(ASTAllocator& allocator, bool parseStruct) {

    auto id = consumeIdentifierOrKeyword();
    if(id == nullptr) {
        return nullptr;
    }

    switch(id->type) {
        case TokenType::NullKw:
            return new (allocator.allocate<NullValue>()) NullValue(loc_single(token));
        case TokenType::TrueKw:
            return new (allocator.allocate<BoolValue>()) BoolValue(true, loc_single(token));
        case TokenType::FalseKw:
            return new (allocator.allocate<BoolValue>()) BoolValue(false, loc_single(token));
        default:
            break;
    }

    auto tokenType = token->type;

    switch(tokenType) {
        case TokenType::NewLine:
        case TokenType::RParen:
        case TokenType::RBrace:
        case TokenType::RBracket:
        case TokenType::CommaSym:
            return new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(id->value), loc_single(id));;
        case TokenType::LBrace: {
            auto ref_type = new (allocator.allocate<LinkedType>()) LinkedType(std::string(id->value), loc_single(id));
            return parseStructValue(allocator, ref_type, id->position);
        }
        case TokenType::Whitespace: {
            token++;
            auto tokenType2 = token->type;
            if(tokenType2 == TokenType::LBrace) {
                // StructName {
                auto ref_type = new (allocator.allocate<LinkedType>()) LinkedType(std::string(id->value), loc_single(id));
                return parseStructValue(allocator, ref_type, id->position);
            } else {
                break;
            }
        }
        default:
            break;
    }

    auto chain = new (allocator.allocate<AccessChain>()) AccessChain({}, parent_node, false, 0);
    auto identifier = new (allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(id->value), loc_single(id));
    chain->values.emplace_back(identifier);

    return parseAccessChainAfterId(allocator, chain, id->position, parseStruct);

}

bool Parser::lexAccessChain(bool lexStruct, bool lex_as_node) {

    auto id = consumeIdentifierOrKeyword();
    if(id == nullptr) {
        return false;
    }

    auto creator = ValueCreators.find(id->value);
    if(creator != ValueCreators.end()) {
        // TODO use passed allocator
        return straight_value(creator->second(this, global_allocator, id));
    } else {
        // TODO use passed allocator
        auto value = new (global_allocator.allocate<VariableIdentifier>()) VariableIdentifier(std::string(id->value), loc_single(id));
        straight_value(value);
    }

    auto start = tokens_size() - 1;

    lexAccessChainAfterId(lexStruct);

    if(start < tokens_size() && !unit.tokens[start]->is_struct_value()) {
        compound_from(start, lex_as_node ? LexTokenType::CompAccessChainNode : LexTokenType::CompAccessChain);
    }

    return true;

}

Value* Parser::parseAccessChainOrAddrOf(ASTAllocator& allocator, bool parseStruct) {
    auto token1 = consumeOfType(TokenType::AmpersandSym);
    if(token1) {
        auto chain = parseAccessChain(allocator, true);
        if(chain) {
            return new (allocator.allocate<AddrOfValue>()) AddrOfValue(chain, loc_single(token1));
        } else {
            error("expected a value after '&' for address of");
            return nullptr;
        }
    } else {
        auto token2 = consumeOfType(TokenType::MultiplySym);
        if(token2) {
            auto chain = parseAccessChain(allocator, false);
            if(chain) {
                return new (allocator.allocate<DereferenceValue>()) DereferenceValue(chain, loc_single(token2));
            } else {
                error("expected a value after '*' for dereference");
                return nullptr;
            }
        }
    }
    return parseAccessChain(allocator, parseStruct);
}

bool Parser::lexAccessChainOrAddrOf(bool lexStruct) {
    if(lexOperatorToken(TokenType::AmpersandSym)) {
        auto start = tokens_size() - 1;
        if(lexAccessChain(true)) {
            compound_from(start, LexTokenType::CompAddrOf);
        } else {
            error("expected a value after '&' for address of");
        }
        return true;
    } else if(lexOperatorToken(TokenType::MultiplySym)) {
        auto start = tokens_size() - 1;
        if(lexAccessChain(false)) {
            compound_from(start, LexTokenType::CompDeference);
        } else {
            error("expected a value after '*' for dereference");
        }
        return true;
    }
    return lexAccessChain(lexStruct);
}

bool Parser::lexAccessChainRecursive(bool lexStruct, unsigned chain_length) {
    if (!lexVariableToken()) {
        return false;
    }
    return lexAccessChainAfterId(lexStruct, chain_length + 1);
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

FunctionCall* Parser::parseFunctionCall(ASTAllocator& allocator) {
    auto lParen = consumeToken(TokenType::LParen);
    if(lParen) {
        auto call = new (allocator.allocate<FunctionCall>()) FunctionCall({}, 0);
        do {
            consumeWhitespaceAndNewLines();
            auto expr = parseExpression(allocator, true);
            if(expr) {
                call->values.emplace_back(expr);
            } else {
                auto init = parseArrayInit(allocator);
                if(init) {
                    call->values.emplace_back(init);
                }
            }
            readWhitespace();
        } while (consumeToken(TokenType::CommaSym));
        consumeWhitespaceAndNewLines();
        if (!consumeToken(TokenType::RParen)) {
            error("expected a ')' for a function call, after starting '('");
            return call;
        }
        return call;
    } else {
        return nullptr;
    }
}

bool Parser::lexFunctionCall(unsigned back_start) {
    if(lexOperatorToken(TokenType::LParen)) {
        unsigned start = tokens_size() - back_start;
        do {
            lexWhitespaceAndNewLines();
            if (!(lexExpressionTokens(true) || lexArrayInit())) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(TokenType::CommaSym));
        lexWhitespaceAndNewLines();
        if (!lexOperatorToken(TokenType::RParen)) {
            error("expected a ')' for a function call, after starting '('");
            return true;
        }
        compound_from(start, LexTokenType::CompFunctionCall);
        return true;
    } else {
        return false;
    }
}

void Parser::lexGenericArgsList() {
    do {
        lexWhitespaceToken();
        if (!lexTypeTokens()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(TokenType::CommaSym));
}

bool Parser::lexGenericArgsListCompound() {
    if(lexOperatorToken(TokenType::LessThanSym)) {
        unsigned start = tokens_size() - 1;
        lexGenericArgsList();
        if (!lexOperatorToken(TokenType::GreaterThanSym)) {
            error("expected a '>' for generic list in function call");
            return true;
        }
        compound_from(start, LexTokenType::CompGenericList);
        return true;
    } else {
        return false;
    }
}

void Parser::parseGenericArgsList(std::vector<BaseType*>& outArgs, ASTAllocator& allocator) {
    if(consumeToken(TokenType::LessThanSym)) {
        do {
            readWhitespace();
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

void Parser::lexFunctionCallWithGenericArgsList() {
    lexGenericArgsListCompound();
    if(token->type == TokenType::LParen){
        lexFunctionCall(2);
    } else {
        error("expected a '(' after the generic list in function call");
    }
}

BaseType* Parser::ref_type_from(ASTAllocator& allocator, AccessChain* chain) {
    if(chain->values.size() == 1) {
        auto val = (VariableIdentifier*) chain->values.back();
        return new (allocator.allocate<LinkedType>()) LinkedType(val->value, val->location);
    } else {
        return new (allocator.allocate<LinkedValueType>()) LinkedValueType(chain, chain->location);
    }
}

Value* Parser::parseAccessChainAfterId(ASTAllocator& allocator, AccessChain* chain, Position& start, bool parseStruct) {

    if(parseStruct) {
        readWhitespace();
        if(token->type == TokenType::LBrace) {
            return parseStructValue(allocator, ref_type_from(allocator, chain), start);
        }
    }

    // when there is generic args after the identifier StructName<int, float> or func_name<int, float>()
    if (token->type == TokenType::LessThanSym) {
        std::vector<BaseType*> genArgs;
        parseGenericArgsList(genArgs, allocator);
        readWhitespace();
        if(token->type == TokenType::LParen) {
            auto call = parseFunctionCall(allocator);
            call->generic_list = std::move(genArgs);
            chain->values.emplace_back(call);
        } else if(parseStruct && token->type == TokenType::LBrace) {
            if(chain->values.size() == 1) {
                auto id = (VariableIdentifier*) chain->values.back();
                auto ref_type = new (allocator.allocate<LinkedType>()) LinkedType(std::string(id->value), id->location);
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
        std::vector<Value*> indexOpValues;
        auto firstLBracket = consumeOfType(TokenType::LBracket);
        Token* closingToken = firstLBracket;
        do {
            readWhitespace();
            auto expr = parseExpression(allocator);
            if(!expr) {
                error("expected an expression in indexing operators for access chain");
                auto index_op = new (allocator.allocate<IndexOperator>()) IndexOperator(indexOpValues, loc(firstLBracket, closingToken));
                chain->values.emplace_back(index_op);
                return chain;
            }
            readWhitespace();
            if (!consumeToken(TokenType::RBracket)) {
                error("expected a closing bracket ] in access chain");
                auto index_op = new (allocator.allocate<IndexOperator>()) IndexOperator(indexOpValues, loc(firstLBracket, closingToken));
                chain->values.emplace_back(index_op);
                return chain;
            }
            indexOpValues.emplace_back(expr);
        } while (consumeToken(TokenType::LBracket));
        auto indexOp = new (allocator.allocate<IndexOperator>()) IndexOperator(std::move(indexOpValues), loc(firstLBracket, closingToken));
        chain->values.emplace_back(indexOp);
        while(true) {
            if (token->type == TokenType::LParen) {
                auto call = parseFunctionCall(allocator);
                chain->values.emplace_back(call);
            } else if(token->type == TokenType::LessThanSym) {
                std::vector<BaseType*> genArgs;
                parseGenericArgsList(genArgs, allocator);
                if(token->type == TokenType::LParen){
                    auto call = parseFunctionCall(allocator);
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
    if(lexOperatorToken(TokenType::DotSym) && !parseAccessChainRecursive(allocator, chain, start, false)) {
        error("expected a identifier after the dot . in the access chain");
        return chain;
    } else if(lexOperatorToken(TokenType::DoubleColonSym) && !parseAccessChainRecursive(allocator, chain, start, parseStruct)) {
        error("expected a identifier after the :: in the access chain");
        return chain;
    }

    return chain;

}

bool Parser::lexAccessChainAfterId(bool lexStruct, unsigned chain_length) {

    if(lexStruct) {
        lexWhitespaceToken();
        if(token->type == TokenType::LBrace) {
            if(chain_length > 1) {
                compound_from(tokens_size() - chain_length, LexTokenType::CompAccessChain);
            }
            return lexStructValueTokens(1);
        }
    }

    // when there is generic args after the identifier StructName<int, float> or func_name<int, float>()
    if (token->type == TokenType::LessThanSym && isGenericEndAhead()) {
        lexGenericArgsListCompound();
        lexWhitespaceToken();
        if(token->type == TokenType::LParen) {
            lexFunctionCall(2);
        } else if(lexStruct && token->type == TokenType::LBrace) {
            if(chain_length > 1) {
                compound_from(tokens_size() - chain_length, LexTokenType::CompAccessChain);
            }
            return lexStructValueTokens(2);
        } else {
            error("expected a '(' or '{' after the generic list for a function call or struct initialization");
        }
    }

    while(token->type == TokenType::LParen || token->type == TokenType::LBracket) {
        while(lexOperatorToken(TokenType::LBracket)) {
            unsigned start = tokens_size() - 1;
            do {
                lexWhitespaceToken();
                if (!lexExpressionTokens()) {
                    error("expected an expression in indexing operators for access chain");
                    return true;
                }
                lexWhitespaceToken();
                if (!lexOperatorToken(TokenType::RBracket)) {
                    error("expected a closing bracket ] in access chain");
                    return true;
                }
            } while (lexOperatorToken(TokenType::LBracket));
            compound_from(start, LexTokenType::CompIndexOp);
        }
        while(true) {
            if (token->type == TokenType::LParen) {
                lexFunctionCall(1);
            } else if(token->type == TokenType::LessThanSym) {
                lexFunctionCallWithGenericArgsList();
            } else {
                break;
            }
        }
    }

    if(lexOperatorToken(TokenType::DotSym) && !lexAccessChainRecursive(false)) {
        error("expected a identifier after the dot . in the access chain");
        return true;
    } else if(lexOperatorToken(TokenType::DoubleColonSym) && !lexAccessChainRecursive(lexStruct, chain_length + 1)) {
        error("expected a identifier after the :: in the access chain");
        return true;
    }

    return true;

}