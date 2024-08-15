// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexLambdaTypeTokens(unsigned int start) {
    if(lexOperatorToken('(')) {
        lexParameterList();
        if(!lexOperatorToken(')')) {
            error("expected a ')' after the ')' in lambda function type");
        }
        lexWhitespaceToken();
        if(lexOperatorToken("=>")) {
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

bool Lexer::lexGenericTypeAfterId(unsigned int start) {
    if(lexOperatorToken('<')) {
        do {
            lexWhitespaceToken();
            if(!lexTypeTokens()) {
                break;
            }
            lexWhitespaceToken();
        } while(lexOperatorToken(','));
        lexWhitespaceToken();
        if(!lexOperatorToken('>')) {
            error("expected '>' for generic type");
        }
        compound_from(start, LexTokenType::CompGenericType);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexRefOrGenericType() {
    unsigned start = tokens.size();
    auto id = provider.readIdentifier();
    if(id.empty()) {
        error("missing struct / interface name in inheritance list of the struct");
        return false;
    }
    tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Type, backPosition(id.length()), id));
    lexWhitespaceToken();
    lexGenericTypeAfterId(start);
    return true;
}

bool Lexer::lexTypeTokens() {

    if(lexOperatorToken('[')) {
        unsigned start = tokens.size() - 1;
        if(!lexOperatorToken(']')) {
            error("expected ']' after '[' for lambda type");
            return true;
        }
        lexWhitespaceToken();
        if(!lexLambdaTypeTokens(start)) {
            error("expected a lambda type after '[]'");
        }
        return true;
    }

    if(lexLambdaTypeTokens(tokens.size())) {
        return true;
    }

    std::string type = provider.readIdentifier();
    if(type.empty()) return false;
    // dyn should be a keyword
    if(type == "dyn") {
        if(lexWhitespaceToken()) {
            unsigned prev_start = tokens.size();
            tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Type, backPosition(type.length()), type));
            std::string contained_type = provider.readIdentifier();
            if(contained_type.empty()) {
                error("expected a type identifier after dyn keyword");
                return true;
            }
            unsigned start = tokens.size();
            tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Type, backPosition(contained_type.length()), contained_type));
            lexGenericTypeAfterId(start);
            compound_from(prev_start, LexTokenType::CompSpecializedType);
            return true;
        }
    }
    bool has_multiple = false;
    unsigned start = tokens.size();
    while(true) {
        if(provider.peek() == ':' && provider.peek(1) == ':') {
            tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Variable, backPosition(type.length()), type));
            lexOperatorToken("::");
            auto new_type = provider.readIdentifier();
            if(new_type.empty()) {
                error("expected an identifier after '" + type + "::' for a type");
                return true;
            } else {
                has_multiple = true;
                type = new_type;
            }
        } else {
            if(has_multiple) {
                tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Variable, backPosition(type.length()), type));
                compound_from(start, LexTokenType::CompAccessChain);
                compound_from(start, LexTokenType::CompReferencedValueType);
            } else {
                tokens.emplace_back(std::make_unique<LexToken>(LexTokenType::Type, backPosition(type.length()), type));
            }
            break;
        }
    }

    lexGenericTypeAfterId(start);
    if(lexOperatorToken('[')) {
        // optional array size
        lexUnsignedIntAsNumberToken();
        if(!lexOperatorToken(']')) {
            error("expected ']' for array type");
        }
        compound_from(start, LexTokenType::CompArrayType);
    }
    while(lexOperatorToken('*')) {
        compound_from(start, LexTokenType::CompPointerType);
    }

    return true;

}