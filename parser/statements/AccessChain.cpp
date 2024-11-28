// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "parser/Parser.h"
#include "ast/values/BoolValue.h"
#include "ast/values/NullValue.h"
#include "ast/values/VariableIdentifier.h"

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

Token* Parser::consumeOfType(TokenType type) {
    auto& t = *token;
    if(t.type == type) {
        token++;
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

void Parser::lexFunctionCallWithGenericArgsList() {
    lexGenericArgsListCompound();
    if(token->type == TokenType::LParen){
        lexFunctionCall(2);
    } else {
        error("expected a '(' after the generic list in function call");
    }
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