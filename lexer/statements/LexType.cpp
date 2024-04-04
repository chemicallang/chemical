// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/TypeToken.h"

bool Lexer::lexTypeTokens() {

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
        return true;
    }

    auto type = lexAnything([&] () -> bool {
        return std::isalpha(provider.peek());
    });
    if (!type.empty()) {
        tokens.emplace_back(std::make_unique<TypeToken>(backPosition(type.length()), type));
        if(lexOperatorToken('<')) {
            if(!lexTypeTokens()) {
                error("expected a type within '<' '>' for generic type");
            }
            if(!lexOperatorToken('>')) {
                error("expected '>' for generic type");
            }
        } else if(lexOperatorToken('[')) {
            // optional array size
            lexUnsignedIntAsNumberToken();
            if(!lexOperatorToken(']')) {
                error("expected ']' for array type");
            }
        }
        lexOperatorToken('*');
        return true;
    } else {
        return false;
    }
}