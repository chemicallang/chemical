// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/utils/ValueCreators.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "lexer/model/tokens/IdentifierToken.h"
#include "cst/values/AccessChainCST.h"
#include "cst/values/FunctionCallCST.h"
#include "cst/values/AddrOfCST.h"
#include "cst/values/DereferenceCST.h"
#include "cst/values/IndexOpCST.h"
#include "cst/statements/GenericListCST.h"

bool Lexer::storeVariable(const std::string& identifier) {
    if (!identifier.empty()) {
        tokens.emplace_back(std::make_unique<VariableToken>(LexTokenType::Variable, backPosition(identifier.length()), identifier));
        return true;
    } else {
        return false;
    }
}

bool Lexer::storeIdentifier(const std::string &identifier) {
    if (!identifier.empty()) {
        tokens.emplace_back(std::make_unique<IdentifierToken>(LexTokenType::Identifier, backPosition(identifier.length()), identifier));
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
        compound_from<AccessChainCST>(start, LexTokenType::CompAccessChain);
    }

    return true;

}

bool Lexer::lexAccessChainOrAddrOf(bool lexStruct) {
    if(lexOperatorToken('&')) {
        auto start = tokens.size() - 1;
        lexAccessChain(false);
        compound_from<AddrOfCST>(start, LexTokenType::CompAddrOf);
        return true;
    } else if(lexOperatorToken('*')) {
        auto start = tokens.size() - 1;
        lexAccessChain(false);
        compound_from<DereferenceCST>(start, LexTokenType::CompDeference);
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

void Lexer::lexFunctionCallAfterLParen(unsigned back_start) {
    unsigned start = tokens.size() - back_start;
    do {
        lexWhitespaceToken();
        if(!(lexExpressionTokens(true) || lexArrayInit())) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(','));
    if(!lexOperatorToken(')')) {
        error("expected a ')' for a function call, after starting ')'");
    }
    compound_from<FunctionCallCST>(start, LexTokenType::CompFunctionCall);
}

void Lexer::lexFunctionCallAfterGenericStart() {
    unsigned start = tokens.size() - 1;
    do {
        lexWhitespaceToken();
        if (!lexTypeTokens()) {
            break;
        }
        lexWhitespaceToken();
    } while (lexOperatorToken(','));
    if(!lexOperatorToken('>')) {
        error("expected a '>' for generic list in function call");
    }
    compound_from<GenericListCST>(start, LexTokenType::CompGenericList);
    if(lexOperatorToken('(')){
        lexFunctionCallAfterLParen(2);
    } else {
        error("expected a '(' after the generic list in function call");
    }
}

bool Lexer::lexAccessChainAfterId(bool lexStruct, unsigned chain_length) {

    if(lexStruct) {
        lexWhitespaceToken();
        if(provider.peek() == '{') {
            if(chain_length > 1) {
                compound_from<AccessChainCST>(tokens.size() - chain_length, LexTokenType::CompAccessChain);
            }
            return lexStructValueTokens();
        }
    }

    if (provider.peek() == '<' && isGenericEndAhead()) {
        lexOperatorToken('<');
        lexFunctionCallAfterGenericStart();
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
            compound_from<IndexOpCST>(start, LexTokenType::CompIndexOp);
        }
        while(true) {
            if (lexOperatorToken('(')) {
                lexFunctionCallAfterLParen(1);
            } else if(lexOperatorToken('<')) {
                lexFunctionCallAfterGenericStart();
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