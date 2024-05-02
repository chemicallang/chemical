// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/VariableToken.h"
#include "cst/values/AccessChainCST.h"
#include "cst/values/FunctionCallCST.h"
#include "cst/values/AddrOfCST.h"
#include "cst/values/DereferenceCST.h"
#include "cst/values/IndexOpCST.h"

bool Lexer::storeIdentifier(const std::string& identifier) {
    if (!identifier.empty()) {
        tokens.emplace_back(std::make_unique<VariableToken>(backPosition(identifier.length()), identifier));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChain(bool lexStruct) {

    if (!lexIdentifierToken()) {
        return false;
    }

    auto start = tokens.size() - 1;

    lexAccessChainAfterId(lexStruct);

    if(!tokens[start]->is_struct_value()) {
        compound_from<AccessChainCST>(start);
    }

    return true;

}

bool Lexer::lexAccessChainOrAddrOf(bool lexStruct) {
    if(lexOperatorToken('&')) {
        auto start = tokens.size() - 1;
        lexAccessChain(false);
        compound_from<AddrOfCST>(start);
        return true;
    } else if(lexOperatorToken('*')) {
        auto start = tokens.size() - 1;
        lexAccessChain(false);
        compound_from<DereferenceCST>(start);
        return true;
    }
    return lexAccessChain(lexStruct);
}

bool Lexer::lexAccessChainRecursive(bool lexStruct) {
    if (!lexIdentifierToken()) {
        return false;
    }
    return lexAccessChainAfterId(lexStruct);
}

bool Lexer::lexIndexOp() {
    if(lexOperatorToken('[')) {
        unsigned start = tokens.size() - 2;
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
        } while(lexOperatorToken('['));
        compound_from<IndexOpCST>(start);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChainAfterId(bool lexStruct) {

    if(lexStruct) {
        lexWhitespaceToken();
        if(provider.peek() == '{') {
            return lexStructValueTokens();
        }
    }

    lexIndexOp();

    if (lexOperatorToken('(')) {
        unsigned start = tokens.size() - 2;
        do {
            lexWhitespaceToken();
            if(!lexExpressionTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while (lexOperatorToken(','));
        if(!lexOperatorToken(')')) {
            error("expected a ')' for a function call, after starting ')'");
        }
        compound_from<FunctionCallCST>(start);
    }

    lexIndexOp();

    while (lexOperatorToken('.')) {
        if (!lexAccessChainRecursive(false)) {
            error("expected a identifier after the dot . in the access chain");
            return true;
        }
    }

    return true;

}