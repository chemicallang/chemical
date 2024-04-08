// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 2/19/2024.
//

#include "lexer/Lexer.h"
#include "lexer/model/tokens/TypeToken.h"
#include "cst/types/FunctionTypeCST.h"
#include "cst/types/ArrayTypeCST.h"
#include "cst/types/PointerTypeCST.h"
#include "cst/types/GenericTypeCST.h"

bool Lexer::lexTypeTokens() {

    if(lexOperatorToken('(')) {
        unsigned start = tokens.size() - 1;
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
        if(isCST()) {
            compound<FunctionTypeCST>(start);
        }
        return true;
    }

    auto type = lexAnything([&] () -> bool {
        return std::isalpha(provider.peek());
    });
    if (!type.empty()) {
        unsigned start = tokens.size();
        tokens.emplace_back(std::make_unique<TypeToken>(backPosition(type.length()), type));
        if(lexOperatorToken('<')) {
            if(!lexTypeTokens()) {
                error("expected a type within '<' '>' for generic type");
            }
            if(!lexOperatorToken('>')) {
                error("expected '>' for generic type");
            }
            if(isCST()) {
                compound<GenericTypeCST>(start);
            }
        } else if(lexOperatorToken('[')) {
            // optional array size
            lexUnsignedIntAsNumberToken();
            if(!lexOperatorToken(']')) {
                error("expected ']' for array type");
            }
            if(isCST()) {
                compound<ArrayTypeCST>(start);
            }
        }
        if(lexOperatorToken('*')) {
            if(isCST()) {
                compound<PointerTypeCST>(start);
            }
        }
        return true;
    } else {
        return false;
    }
}