// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 26/02/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include "lexer/model/tokens/KeywordToken.h"
#include "lexer/model/tokens/StringOperatorToken.h"
#include "lexer/model/tokens/OperationToken.h"

bool Lexer::lexOperatorToken(char op) {
    if(provider.increment(op)) {
        tokens.emplace_back(std::make_unique<CharOperatorToken>(backPosition(1), std::string(1, op)));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexOperatorToken(const std::string& op) {
    if(provider.increment(op)) {
        tokens.emplace_back(std::make_unique<StringOperatorToken>(backPosition(op.length()), op));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexOperationToken(char token, Operation op) {
    if(provider.increment(token)) {
        std::string value;
        value.append(std::to_string((int) op));
        value.append(1, token);
        tokens.emplace_back(std::make_unique<OperationToken>(backPosition(1), std::move(value)));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexOperatorToken(const std::string &token, Operation op) {
    if(provider.increment(token)) {
        std::string value;
        value.append(std::to_string((int) op));
        value.append(token);
        tokens.emplace_back(std::make_unique<OperationToken>(backPosition(token.length()), value));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexKeywordToken(const std::string& keyword) {
    if(provider.increment(keyword)) {
        tokens.emplace_back(std::make_unique<KeywordToken>(backPosition(keyword.length()), keyword));
        return true;
    } else {
        return false;
    }
}