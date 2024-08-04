// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/utils/ValueCreators.h"

bool Lexer::storeVariable(const std::string& identifier) {
    if (!identifier.empty()) {
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Variable, backPosition(identifier.length()), identifier));
        return true;
    } else {
        return false;
    }
}

bool Lexer::storeIdentifier(const std::string &identifier) {
    if (!identifier.empty()) {
        tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Identifier, backPosition(identifier.length()), identifier));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChain(bool lexStruct) {

    auto id = lexIdentifier();
    if(id.empty()) {
        return false;
    }

    auto creator = ValueCreators.find(id);
    if(creator != ValueCreators.end()) {
        creator->second(this);
        return true;
    } else {
        storeVariable(id);
    }

    auto start = tokens.size() - 1;

    lexAccessChainAfterId(lexStruct);

    if(!tokens[start]->is_struct_value()) {
        compound_from(start, LexTokenType::CompAccessChain);
    }

    return true;

}

bool Lexer::lexAccessChainOrAddrOf(bool lexStruct) {
    if(lexOperatorToken('&')) {
        auto start = tokens.size() - 1;
        lexAccessChain(false);
        compound_from(start, LexTokenType::CompAddrOf);
        return true;
    } else if(lexOperatorToken('*')) {
        auto start = tokens.size() - 1;
        lexAccessChain(false);
        compound_from(start, LexTokenType::CompDeference);
        return true;
    }
    return lexAccessChain(lexStruct);
}

bool Lexer::lexAccessChainRecursive(bool lexStruct, unsigned chain_length) {
    if (!lexVariableToken()) {
        return false;
    }
    return lexAccessChainAfterId(lexStruct, chain_length + 1);
}

bool Lexer::lexFunctionCall(unsigned back_start) {
    if(lexOperatorToken('(')) {
        unsigned start = tokens.size() - back_start;
        do {
            lexWhitespaceAndNewLines();
            if (!(lexExpressionTokens(true) || lexArrayInit())) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(','));
        lexWhitespaceAndNewLines();
        if (!lexOperatorToken(')')) {
            error("expected a ')' for a function call, after starting '('");
            return true;
        }
        compound_from(start, LexTokenType::CompFunctionCall);
        return true;
    } else {
        return false;
    }
}

void Lexer::lexGenericArgsList() {
    do {
        lexWhitespaceToken();
        if (!lexTypeTokens()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(','));
}

bool Lexer::lexGenericArgsListCompound() {
    if(lexOperatorToken('<')) {
        unsigned start = tokens.size() - 1;
        lexGenericArgsList();
        if (!lexOperatorToken('>')) {
            error("expected a '>' for generic list in function call");
            return true;
        }
        compound_from(start, LexTokenType::CompGenericList);
        return true;
    } else {
        return false;
    }
}

void Lexer::lexFunctionCallWithGenericArgsList() {
    lexGenericArgsListCompound();
    if(provider.peek() == '('){
        lexFunctionCall(2);
    } else {
        error("expected a '(' after the generic list in function call");
    }
}

bool Lexer::lexAccessChainAfterId(bool lexStruct, unsigned chain_length) {

    if(lexStruct) {
        lexWhitespaceToken();
        if(provider.peek() == '{') {
            if(chain_length > 1) {
                compound_from(tokens.size() - chain_length, LexTokenType::CompAccessChain);
            }
            return lexStructValueTokens(1);
        }
    }

    // when there is generic args after the identifier StructName<int, float> or func_name<int, float>()
    if (provider.peek() == '<' && isGenericEndAhead()) {
        lexGenericArgsListCompound();
        lexWhitespaceToken();
        if(provider.peek() == '(') {
            lexFunctionCall(2);
        } else if(lexStruct && provider.peek() == '{') {
            if(chain_length > 1) {
                compound_from(tokens.size() - chain_length, LexTokenType::CompAccessChain);
            }
            return lexStructValueTokens(2);
        } else {
            error("expected a '(' or '{' after the generic list for a function call or struct initialization");
        }
    }

    while(provider.peek() == '(' || provider.peek() == '[') {
        while(lexOperatorToken('[')) {
            unsigned start = tokens.size() - 1;
            do {
                lexWhitespaceToken();
                if (!lexExpressionTokens()) {
                    error("expected an expression in indexing operators for access chain");
                    return true;
                }
                lexWhitespaceToken();
                if (!lexOperatorToken(']')) {
                    error("expected a closing bracket ] in access chain");
                    return true;
                }
            } while (lexOperatorToken('['));
            compound_from(start, LexTokenType::CompIndexOp);
        }
        while(true) {
            if (provider.peek() == '(') {
                lexFunctionCall(1);
            } else if(provider.peek() == '<') {
                lexFunctionCallWithGenericArgsList();
            } else {
                break;
            }
        }
    }

    if(lexOperatorToken('.') && !lexAccessChainRecursive(false)) {
        error("expected a identifier after the dot . in the access chain");
        return true;
    } else if(lexOperatorToken("::") && !lexAccessChainRecursive(lexStruct, chain_length + 1)) {
        error("expected a identifier after the :: in the access chain");
        return true;
    }

    return true;

}