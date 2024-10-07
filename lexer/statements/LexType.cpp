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
    unsigned start = tokens_size();
    auto id = provider.readIdentifier();
    if(id.empty()) {
        error("missing struct / interface name in inheritance list of the struct");
        return false;
    }
    emplace(LexTokenType::Type, backPosition(id.length()), id);
    lexWhitespaceToken();
    lexGenericTypeAfterId(start);
    return true;
}

void Lexer::lexArrayAndPointerTypesAfterTypeId(unsigned int start) {
    if(lexOperatorToken('[')) {
        // optional array size
        lexUnsignedIntAsNumberToken();
        if(!lexOperatorToken(']')) {
            error("expected ']' for array type");
            return;
        }
        compound_from(start, LexTokenType::CompArrayType);
    }
    while(lexOperatorToken('*')) {
        compound_from(start, LexTokenType::CompPointerType);
    }
}

bool Lexer::lexTypeId(std::string& type, unsigned int start) {
    bool has_multiple = false;
    while(true) {
        if(provider.peek() == ':' && provider.peek(1) == ':') {
            emplace(LexTokenType::Variable, backPosition(type.length()), type);
            lexOperatorToken("::");
            auto new_type = provider.readIdentifier();
            if(new_type.empty()) {
                error("expected an identifier after '" + type + "::' for a type");
                return false;
            } else {
                has_multiple = true;
                type = new_type;
            }
        } else {
            if(has_multiple) {
                emplace(LexTokenType::Variable, backPosition(type.length()), type);
                compound_from(start, LexTokenType::CompAccessChain);
                compound_from(start, LexTokenType::CompLinkedValueType);
            } else {
                emplace(LexTokenType::Type, backPosition(type.length()), type);
            }
            break;
        }
    }
    return true;
}

bool Lexer::lexTypeTokens() {

    if(lexOperatorToken('[')) {
        unsigned start = tokens_size() - 1;
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

    if(lexLambdaTypeTokens(tokens_size())) {
        return true;
    }

    bool lexed_ref = lexOperatorToken('&');
    std::string type = provider.readIdentifier();
    if(type.empty()) return false;
    // dyn should be a keyword
    if(type == "dyn" && lexWhitespaceToken()) {
        unsigned prev_start = tokens_size();
        emplace(LexTokenType::Type, backPosition(type.length()), type);
        std::string contained_type = provider.readIdentifier();
        if(contained_type.empty()) {
            error("expected a type identifier after dyn keyword");
            return true;
        }
        unsigned start = tokens_size();
        if(!lexTypeId(contained_type, start)) {
            return true;
        }
        lexGenericTypeAfterId(start);
        compound_from(prev_start, LexTokenType::CompSpecializedType);
        lexArrayAndPointerTypesAfterTypeId(prev_start);
        return true;
    }
    unsigned start = tokens_size();
    if(!lexTypeId(type, start)) {
        return true;
    }
    lexGenericTypeAfterId(start);
    lexArrayAndPointerTypesAfterTypeId(start);
    if(lexed_ref) {
        // start - 1, to include '&'
        compound_from(start - 1, LexTokenType::CompReferenceType);
    }

    return true;

}